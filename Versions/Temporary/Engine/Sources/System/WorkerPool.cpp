#include "stdafx.h"

#include "WorkerPool.h"

#include <condition_variable>
#include <deque>
#include <exception>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

struct CWorkerPool::SImpl
{
	mutable std::mutex mutex;
	std::condition_variable workReady;
	std::condition_variable allDone;
	std::deque<TTask> tasks;
	std::vector<std::thread> threads;
	std::size_t nOutstanding = 0;
	bool bStopping = false;
	std::exception_ptr firstException;

	void WorkerLoop()
	{
		for ( ;; )
		{
			TTask task;
			{
				std::unique_lock<std::mutex> lock( mutex );
				workReady.wait( lock, [this] { return bStopping || !tasks.empty(); } );
				if ( bStopping && tasks.empty() )
					return;

				task = std::move( tasks.front() );
				tasks.pop_front();
			}

			try
			{
				task();
			}
			catch ( ... )
			{
				std::lock_guard<std::mutex> lock( mutex );
				if ( !firstException )
					firstException = std::current_exception();
			}

			{
				std::lock_guard<std::mutex> lock( mutex );
				ASSERT( nOutstanding > 0 );
				--nOutstanding;
				if ( nOutstanding == 0 )
					allDone.notify_all();
			}
		}
	}

	void StopThreads()
	{
		{
			std::lock_guard<std::mutex> lock( mutex );
			bStopping = true;
		}
		workReady.notify_all();
		for ( std::thread &thread : threads )
			thread.join();
		threads.clear();
		bStopping = false;
	}

	void StartThreads( std::size_t nThreads )
	{
		threads.reserve( nThreads );
		try
		{
			for ( std::size_t k = 0; k < nThreads; ++k )
				threads.emplace_back( [this] { WorkerLoop(); } );
		}
		catch ( ... )
		{
			StopThreads();
			throw;
		}
	}
};

CWorkerPool::CWorkerPool( std::size_t nThreads )
	: pImpl( std::make_unique<SImpl>() )
{
	pImpl->StartThreads( nThreads );
}

CWorkerPool::~CWorkerPool()
{
	try
	{
		WaitForAll();
	}
	catch ( ... )
	{
		// Destruction still has to join every worker; task errors are reported by normal waits.
	}
	pImpl->StopThreads();
}

void CWorkerPool::AddTask( TTask task )
{
	{
		std::unique_lock<std::mutex> lock( pImpl->mutex );
		ASSERT( !pImpl->bStopping );
		if ( pImpl->threads.empty() )
		{
			lock.unlock();
			task();
			return;
		}

		pImpl->tasks.push_back( std::move( task ) );
		++pImpl->nOutstanding;
	}
	pImpl->workReady.notify_one();
}

void CWorkerPool::WaitForAll()
{
	std::exception_ptr exception;
	{
		std::unique_lock<std::mutex> lock( pImpl->mutex );
		pImpl->allDone.wait( lock, [this] { return pImpl->nOutstanding == 0; } );
		exception = pImpl->firstException;
		pImpl->firstException = nullptr;
	}
	if ( exception )
		std::rethrow_exception( exception );
}

void CWorkerPool::SetThreadCount( std::size_t nThreads )
{
	WaitForAll();
	if ( GetThreadCount() == nThreads )
		return;
	pImpl->StopThreads();
	pImpl->StartThreads( nThreads );
}

std::size_t CWorkerPool::GetThreadCount() const
{
	std::lock_guard<std::mutex> lock( pImpl->mutex );
	return pImpl->threads.size();
}
