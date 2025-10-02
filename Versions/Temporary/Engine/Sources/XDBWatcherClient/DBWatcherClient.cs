using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Tcp;

[assembly:AssemblyKeyFile("../../dbwatcherclient.snk")]
[assembly:AssemblyVersion("1.0.0.0")]

[InterfaceType( ComInterfaceType.InterfaceIsDual )]
[GuidAttribute("dcf300af-6fb9-4537-8978-2fcb77d311dd")]
public struct IDBWatcherClient
{
	void ConnectWatcher();
	bool DumpAllRefs( string szFileName );
	bool CollectRefFiles( string szFileName, out int filesCount );
	string GetRef( int refNumber );

	bool CanSetPause();
	void Pause( bool bPause );
};

[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ProgId("Nival.XdbWatcher")]
[GuidAttribute("79a32419-46a1-4f8b-b044-13b69dad4ca0")]
public class CDBWatcherClient : IDBWatcherClient
{
	private WellKnownClientTypeEntry entry;
	private	string[] refFiles = null;

	public void ConnectWatcher()
	{
		entry = new WellKnownClientTypeEntry(
			typeof(CRemoteRefsAnswerer),
			"tcp://localhost:4300/CRemoteRefsAnswerer" );
		RemotingConfiguration.RegisterWellKnownClientType(entry);
	}

	public bool DumpAllRefs( string szFileName )
	{
		CRemoteRefsAnswerer remoteRefsAnswerer = new CRemoteRefsAnswerer();
		return remoteRefsAnswerer.DumpAllRefs( szFileName );
	}

	public bool CollectRefFiles( string szFileName, out int filesCount )
	{
		CRemoteRefsAnswerer remoteRefsAnswerer = new CRemoteRefsAnswerer();
		refFiles = null;
		filesCount = 0;
		
		bool bSuccess = remoteRefsAnswerer.GetRefsTo( szFileName, out refFiles );
		filesCount = bSuccess ? refFiles.Length : 0;

		return bSuccess;
	}

	public string GetRef( int refNumber )
	{
		if ( refFiles == null || refNumber >= refFiles.Length )
			return null;
		else
			return refFiles[refNumber];
	}

	public bool CanSetPause()
	{
		CRemoteRefsAnswerer remoteRefsAnswerer = new CRemoteRefsAnswerer();
		return remoteRefsAnswerer.CanSetPause();
	}

	public void Pause( bool bPause )
	{
		CRemoteRefsAnswerer remoteRefsAnswerer = new CRemoteRefsAnswerer();
		remoteRefsAnswerer.Pause( bPause );
	}
};
