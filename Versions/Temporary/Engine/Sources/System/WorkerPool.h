#pragma once

#include "System_export.h"

#include <cstddef>
#include <functional>
#include <memory>

// Small persistent worker pool for CPU-only jobs submitted by one master thread.
class SYSTEM_EXPORT CWorkerPool
{
public:
	using TTask = std::function<void()>;

	explicit CWorkerPool( std::size_t nThreads );
	~CWorkerPool();

	CWorkerPool( const CWorkerPool& ) = delete;
	CWorkerPool& operator=( const CWorkerPool& ) = delete;

	void AddTask( TTask task );
	void WaitForAll();

	// Reconfiguration is allowed only while the pool has no outstanding work.
	void SetThreadCount( std::size_t nThreads );
	std::size_t GetThreadCount() const;

private:
	struct SImpl;
	std::unique_ptr<SImpl> pImpl;
};
