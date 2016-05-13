// CommPort.cpp : implementation file
//

#include "stdafx.h"
#include "CommPort.h"


// CCommPort

// ===========================================================
//                  constructor & destructor
// ===========================================================
CCommPort::CCommPort()
{
	m_hComm = INVALID_HANDLE_VALUE;
	m_hThread = NULL; 
	lReqTerminate = 0;
}

CCommPort::~CCommPort()
{
	if ( IsOpen() )
		Close();
}

// ===========================================================
//				   opening and closing ports 
// ===========================================================
BOOL CCommPort::Open ( CString sPortName, 
					   DWORD dwBaudRate, 
					   BYTE bDataBits, 
			           BYTE stopbit, 
					   BYTE parity, 
					   BYTE fc )
{
	if ( IsOpen() ) return FALSE;
		
	CString sPort = _T("\\\\.\\");
	sPort += sPortName;
	m_hComm = CreateFile( sPort,
                    GENERIC_READ | GENERIC_WRITE,
                    0,    // must be opened with exclusive-access
                    NULL, // no security attributes
                    OPEN_EXISTING, // must use OPEN_EXISTING
                    FILE_FLAG_OVERLAPPED, 
                    NULL  // hTemplate must be NULL for comm devices
                    );

	TRACE ( _T("Error: %d\n"), GetLastError() );
	
   if (m_hComm == INVALID_HANDLE_VALUE) 
   {
		TRACE  (_T("CreateFile failed with error %d.\n"), GetLastError());		

		CloseHandle( m_hComm );
		m_hComm = INVALID_HANDLE_VALUE;
	    return FALSE;
   }

   // reconfigure port 
   DCB dcb; 
   BOOL fSuccess = GetCommState(m_hComm, &dcb);

   if (!fSuccess) 
   {
      TRACE (_T("GetCommState failed with error %d.\n"), GetLastError());

	  CloseHandle( m_hComm );
	  m_hComm = INVALID_HANDLE_VALUE;
      return FALSE;
   }
   
   dcb.BaudRate = dwBaudRate;  
   dcb.ByteSize = bDataBits;
   dcb.Parity   = parity;
     
   switch ( stopbit )
   {
    case OneStopBit:      dcb.StopBits = ONESTOPBIT;   break;
    case OneFiveStopBits: dcb.StopBits = ONE5STOPBITS; break;
    case TwoStopBits:     dcb.StopBits = TWOSTOPBITS;  break; 
   }

   switch ( fc )
   {
	case NoFlowControl: 
		{
			dcb.fInX  = FALSE;
			dcb.fOutX = FALSE;
			dcb.fOutxDsrFlow = FALSE;
			dcb.fOutxCtsFlow = FALSE;
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
			dcb.fRtsControl = RTS_CONTROL_ENABLE;
		} break;
	
	case XonXoffFlowControl:
		{
			dcb.fInX  = TRUE;
			dcb.fOutX = TRUE;
			dcb.fOutxDsrFlow = FALSE;
			dcb.fOutxCtsFlow = FALSE;
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
			dcb.fRtsControl = RTS_CONTROL_ENABLE;
		} break;
	
	case Hardware:
		{
			dcb.fInX  = FALSE;
			dcb.fOutX = FALSE;
			dcb.fOutxDsrFlow  = TRUE;
			dcb.fOutxCtsFlow  = TRUE;
			dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
			dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
		} break;
   } 

   // Install new adjustment
   fSuccess = SetCommState(m_hComm, &dcb);
   if (!fSuccess) 
   {
      TRACE (_T("SetCommState failed with error %d.\n"), GetLastError());

	  CloseHandle( m_hComm );
	  m_hComm = INVALID_HANDLE_VALUE;
      return FALSE;
   }

   // launch comm event handler
   DWORD dwId; 
   m_hThread = CreateThread( NULL, 0, EventHandler, (LPVOID)this, CREATE_SUSPENDED, &dwId );

   if ( m_hThread != NULL )
   {
	  m_hEventTerminate = CreateEvent( NULL, FALSE, FALSE, _T("OnStopThread") );
	  lReqTerminate = 1; 
	  ResumeThread( m_hThread );
   }
   else 
   {
	  CloseHandle( m_hComm ); 
	  m_hComm = INVALID_HANDLE_VALUE;
	  return FALSE;
   }
   
   return TRUE;
}

BOOL CCommPort::Open ( CString sPortName, CString sConfig )
{
	if ( IsOpen() ) return FALSE;

	CString sPort = _T("\\\\.\\");
	sPort += sPortName;

	m_hComm = CreateFile( sPort,
                    GENERIC_READ | GENERIC_WRITE,
                    0,    // must be opened with exclusive-access
                    NULL, // no security attributes
                    OPEN_EXISTING, // must use OPEN_EXISTING
                    FILE_FLAG_OVERLAPPED, 
                    NULL  // hTemplate must be NULL for comm devices
                    );

   if (m_hComm == INVALID_HANDLE_VALUE) 
   {
		TRACE  (_T("CreateFile failed with error %d.\n"), GetLastError());		
		   
	   return FALSE;
   }
     
    // execute rearranging the port
   DCB dcb;  

   if ( !GetCommState( m_hComm, &dcb) )
   {
	  TRACE ( _T("GetCommState failed with error %d.\n"), GetLastError());
	  
	  CloseHandle ( m_hComm );  
	  m_hComm = INVALID_HANDLE_VALUE;
      return FALSE;
   }

   if ( !BuildCommDCB ( sConfig, &dcb ) )
   {
	  TRACE( _T("BuildCommDCB failed with error %d.\n"), GetLastError());
	  
	  CloseHandle ( m_hComm );
	  m_hComm = INVALID_HANDLE_VALUE;
      return FALSE;
   }

    // set new settings 
   if ( !SetCommState(m_hComm, &dcb) ) 
   {
      TRACE ( _T("SetCommState failed with error %d.\n"), GetLastError());

	  CloseHandle ( m_hComm );
	  m_hComm = INVALID_HANDLE_VALUE;
      return FALSE;
   }

   // launch comm event handler
   DWORD dwId;
   m_hThread = CreateThread( NULL, 0, EventHandler, (LPVOID)this, CREATE_SUSPENDED, &dwId );
   
   if ( m_hThread != NULL )
   {
	  m_hEventTerminate = CreateEvent( NULL, FALSE, FALSE, _T("OnStopThread") );
	  lReqTerminate = 1; 
	  ResumeThread( m_hThread );
   }
   else 
   {
	  TRACE(  _T( "Thread is fault! Error is %d"), GetLastError()  );
	  		
	  CloseHandle ( m_hComm );
	  m_hComm = INVALID_HANDLE_VALUE;
	  return FALSE;
   }

   return TRUE;
}


// ==============================================================
//							can open
// ==============================================================
BOOL CCommPort::TestOpen( CString sTestingPort )
{	
	CString sPort = _T("\\\\.\\");
	sPort += sTestingPort;
	HANDLE hComm = CreateFile( sPort, GENERIC_READ | GENERIC_WRITE,
                    0, NULL, OPEN_EXISTING, 0, NULL );
	
	BOOL fResult = FALSE;
	if ( hComm != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hComm );
		return TRUE;
	}
	else
		return FALSE;
}
   
// ==============================================================
//							close port 
// ==============================================================
void CCommPort::Close()
{
	PurgeComm( m_hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );
	CancelIo( m_hComm );
	SetCommMask (m_hComm, 0);
			
	if ( m_hThread != NULL )
	{
		InterlockedDecrement( &lReqTerminate );
		Sleep( 0 );
		
		OSVERSIONINFO info;
		info.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

		GetVersionEx ( &info );
			        
		if ( info.dwPlatformId == VER_PLATFORM_WIN32_NT )
		{
			// windows NT family
			while( WaitForSingleObject( m_hEventTerminate, 100 ) != WAIT_OBJECT_0 )
			{
				MSG msg;
				GetMessage(&msg, NULL, 0, 0);
				if ( !TranslateAccelerator(msg.hwnd, NULL, &msg) ) 
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		} 
		else
		{
			// other windows 
            TerminateThread( m_hThread, 0 );
		}

		CloseHandle( m_hComm ); 	
		m_hComm = INVALID_HANDLE_VALUE; 

		CloseHandle( m_hThread );
		CloseHandle( m_hEventTerminate ); 

		m_hThread = NULL;
		m_hEventTerminate = NULL;
		lReqTerminate = 0;
	}
}

// ==============================================================
//				enumerate available ports 
// ==============================================================
DWORD CCommPort::GetAvailablePorts ( CStringList* portList )
{
	//HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM
	HKEY hKey = NULL;

	if ( RegOpenKey( HKEY_LOCAL_MACHINE, _T("Hardware\\DeviceMap\\SerialComm"), &hKey ) != ERROR_SUCCESS )
	{	
		return 0;
	}

	DWORD    cValues;              // number of values for key 
    DWORD    cchMaxValue;          // longest value name 
    DWORD    cbMaxValueData;       // longest value data 
  
	BYTE  bData [100];
	DWORD dwDataLen = 100;
 
	DWORD i, retCode, iFindedPorts=0; 

	TCHAR  achValue[255]; 
	DWORD cchValue = 255; //16300;  //16383
 
    // Get the class name and the value count. 
    retCode = RegQueryInfoKey( hKey, NULL, NULL, NULL, NULL, NULL, NULL, 
								&cValues, &cchMaxValue, &cbMaxValueData, NULL, NULL); 
 
    // enumerate all keys. 
    if (cValues) 
    {
		DWORD dwType;
		for (i=0, retCode=ERROR_SUCCESS; i<cValues; i++) 
        { 
            cchValue = 255; 
            achValue[0] = '\0'; 
			bData [0] = '\0';
			dwDataLen = 100;
            retCode = RegEnumValue(hKey, i, achValue, &cchValue, NULL, &dwType, bData, &dwDataLen );
		
            if (retCode == ERROR_SUCCESS ) 
            { 
				portList ->AddTail( (LPCTSTR)bData );
				iFindedPorts++;
            } 
        }
    }

	return iFindedPorts;
}

// ===========================================================
//				read & write 
// ===========================================================

DWORD CCommPort::Read ( LPVOID lpBuf, DWORD dwCount )
{
  if ( !IsOpen() ) return 0;
  
  OVERLAPPED o;
  memset ( &o, 0, sizeof (OVERLAPPED) );
  o.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL);
  
  DWORD dwBytesRead=0;

  BOOL bResult = ReadFile(m_hComm, lpBuf, dwCount, &dwBytesRead, &o);
  if ( !bResult )
  {
  	 if ( GetLastError() == ERROR_IO_PENDING )		
	 {
		bResult = GetOverlappedResult( m_hComm, &o, &dwBytesRead, TRUE);	 
		return dwBytesRead;
	 }
	 else 
		 return 0;
  }

  CloseHandle( o.hEvent );
  return dwBytesRead;
}

DWORD CCommPort::Write( LPCVOID lpBuf, DWORD dwCount )
{
   if ( !IsOpen() ) return 0;
  
  OVERLAPPED o;
  memset ( &o, 0, sizeof (OVERLAPPED) );
  o.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL);

  DWORD dwBytesWritten = 0;
  BOOL bResult = WriteFile(m_hComm, lpBuf, dwCount, &dwBytesWritten, &o ) ;

  if ( !bResult )
  {
	 if ( GetLastError() == ERROR_IO_PENDING )		
	 {
		bResult = GetOverlappedResult( m_hComm, &o, &dwBytesWritten, TRUE );	 
		if ( !bResult ) 
		{
			//CloseHandle( o.hEvent );
			return 0;
		}
	 }
	 else 
	 {
		 CloseHandle( o.hEvent );
		 return 0;
	 }
  }  
  
  CloseHandle( o.hEvent );
  return dwBytesWritten;
}

// ===========================================================
//						port configuration 
// ===========================================================
BOOL CCommPort::GetConfig(COMMCONFIG& config)
{
	ASSERT( IsOpen() );

	DWORD dwSize = sizeof(COMMCONFIG);

	return GetCommConfig( m_hComm, &config, &dwSize );
}

BOOL CCommPort::SetConfig(COMMCONFIG& config)
{
	ASSERT( IsOpen() );

	return SetCommConfig( m_hComm, &config, sizeof(COMMCONFIG));
}

BOOL CCommPort::GetState(LPDCB dcb)
{
	ASSERT ( IsOpen() );

	return GetCommState( m_hComm, dcb );
}

BOOL CCommPort::SetState(LPDCB dcb)
{
	ASSERT( IsOpen() );

	return SetCommState( m_hComm, dcb );
}

// ===========================================================
//						timeouts
// ===========================================================
BOOL CCommPort::SetTimeouts( LPCOMMTIMEOUTS lpCommTimeouts )
{
	return SetCommTimeouts( m_hComm, lpCommTimeouts );
}

BOOL CCommPort::GetTimeouts( LPCOMMTIMEOUTS lpCommTimeouts ) 
{
	return GetCommTimeouts( m_hComm, lpCommTimeouts );
}

// ===========================================================
//					buffer cleaning 
// ===========================================================
BOOL CCommPort::Purge ( DWORD dwFlags )
{
	ASSERT( IsOpen() );

	return PurgeComm ( m_hComm, dwFlags );
}

BOOL CCommPort::ClearWriteQueue()
{
	return Purge ( PURGE_TXCLEAR );
}

BOOL CCommPort::ClearReadQueue ()
{
	return Purge ( PURGE_RXCLEAR );
}

BOOL CCommPort::AbortAllRead ()
{
	return Purge ( PURGE_RXABORT );
}

BOOL CCommPort::AbortAllWrite ()
{
	return Purge ( PURGE_TXABORT );	
}

BOOL CCommPort::Flush()
{
	ASSERT( IsOpen() );

	return ::FlushFileBuffers( m_hComm );
}

// ===========================================================
//						transmition control 	
// ===========================================================
BOOL CCommPort::TransmitChar( char cChar )
{
	ASSERT( IsOpen() );

	return TransmitCommChar( m_hComm, cChar );
}

BOOL CCommPort::SetBreak()
{
	ASSERT( IsOpen() );

	return SetCommBreak( m_hComm );
}

BOOL CCommPort::ClearBreak()
{
	ASSERT( IsOpen() );

	return ClearCommBreak( m_hComm );
}

BOOL CCommPort::EscapeFunction( DWORD dwFunc )
{
	ASSERT( IsOpen() );

	return EscapeCommFunction( m_hComm, dwFunc );
}

BOOL CCommPort::ClearError( LPDWORD lpErrors, LPCOMSTAT lpStat)
{
	return ClearCommError( m_hComm, lpErrors, lpStat);
}

BOOL CCommPort::GetModemStatus( LPDWORD lpModemStat )
{
	return GetCommModemStatus( m_hComm, lpModemStat );  
}

BOOL CCommPort::GetProperties( LPCOMMPROP lpCommProp )
{
	return GetCommProperties( m_hComm, lpCommProp );
}


// ===========================================================
//						events
// ===========================================================
BOOL CCommPort::SetMask ( DWORD dwMask )
{
    ASSERT( IsOpen() );

	return SetCommMask( m_hComm, dwMask);
}

BOOL CCommPort::GetMask ( DWORD& dwMask )
{
	ASSERT( IsOpen() );

	return GetCommMask( m_hComm, &dwMask );
}

DWORD CCommPort::EventHandler ( LPVOID lpParam )
{
	CCommPort *pPort = (CCommPort*)lpParam;
	DWORD dwMask = 	EV_BREAK | EV_CTS | EV_DSR | EV_ERR | EV_RING | 
					EV_RLSD | EV_RXCHAR | EV_RXFLAG | EV_TXEMPTY; 

	if ( !pPort->SetMask( dwMask ) ) 
	{
		pPort ->OnEventError( COMM_SET_EVNT_MASK_ERROR );
		TRACE ( _T("Thread event handling exiting with error Can't set Mask!\r\n") );
		return 1;
	}

	OVERLAPPED o;	
	DWORD dwResMask=0; 

	o.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    o.Internal = 0;
    o.InternalHigh = 0;
    o.Offset = 0;
    o.OffsetHigh = 0;

	while (1)
	{
		// exit from thread
		if ( pPort->lReqTerminate == 0)
		{
			CloseHandle( o.hEvent );
			SetEvent( pPort->m_hEventTerminate  );
			TRACE ( _T("Thread event handling exiting with 0!\r\n") );
			return 0;      
		}

		// handling comm events
		if ( !WaitCommEvent( pPort->m_hComm, &dwResMask, &o ) )
		{
			DWORD dwError = GetLastError();
			if ( dwError != ERROR_IO_PENDING )
			{
				pPort ->OnEventError( COMM_WAIT_FOR_EVENT_ERROR );
				CloseHandle( o.hEvent );
				SetEvent( pPort->m_hEventTerminate  );
				TRACE ( _T("Thread event handling exiting with 1!\r\n") );
				return 1;
			}
			else
			{
				DWORD dwBytesTransfer;
				GetOverlappedResult( pPort->m_hComm, &o, &dwBytesTransfer, TRUE );
			}
		}
		
		if ( dwResMask & EV_BREAK )
		{
			pPort ->OnBreak();
		}
			
		if ( dwResMask & EV_CTS )
		{
			DWORD dwStatus; 
			pPort ->GetModemStatus( &dwStatus );
			if ( dwStatus & MS_CTS_ON ) 
				pPort ->OnCTS( TRUE );
			else 
				pPort ->OnCTS( FALSE );
		}

		if ( dwResMask & EV_DSR )
		{
			DWORD dwStatus; 
			pPort ->GetModemStatus( &dwStatus );
			if ( dwStatus & MS_DSR_ON ) 
				pPort ->OnDSR( TRUE );
			else 
				pPort ->OnDSR( FALSE );
		}

		if ( dwResMask & EV_ERR )
		{
			DWORD dwError;
			COMSTAT comStat;
			pPort ->ClearError( &dwError, &comStat );
			pPort ->OnERR( dwError );
		}

		if ( dwResMask & EV_RING )
		{
			DWORD dwStatus; 
			pPort ->GetModemStatus( &dwStatus );
			if ( dwStatus & MS_DSR_ON ) 
                pPort ->OnRing( TRUE );
			else 
				pPort ->OnRing( FALSE );
		}
			
		if ( dwResMask & EV_RLSD )
		{
			DWORD dwStatus; 
			pPort ->GetModemStatus( &dwStatus );
			if ( dwStatus & MS_RLSD_ON ) 
                pPort ->OnRlsd( TRUE );
			else 
				pPort ->OnRlsd( FALSE );
		}
			
		if ( dwResMask & EV_RXCHAR )
		{
			DWORD dwError;
			COMSTAT comStat;
			pPort ->ClearError( &dwError, &comStat );
			pPort ->OnRxChar( comStat.cbInQue );
		}
			
		if ( dwResMask & EV_RXFLAG )
		{
			pPort ->OnRxFlag();
		}
				
		if ( dwResMask & EV_TXEMPTY )
		{
			pPort ->OnTxEmpty();
		}
	}  // while 

	CloseHandle( o.hEvent );

	return 0;
}

void CCommPort::OnBreak()  {}
void CCommPort::OnCTS( BOOL fCTS ) {} 
void CCommPort::OnDSR( BOOL fDSR ) {} 
void CCommPort::OnERR( DWORD dwError ) {}
void CCommPort::OnRing( BOOL fRing )  {} 
void CCommPort::OnRlsd( BOOL fLSD )   {}
void CCommPort::OnRxChar( DWORD dwCount ) {}
void CCommPort::OnRxFlag() {}  
void CCommPort::OnTxEmpty(){}
void CCommPort::OnEventError ( DWORD dwErrCode ) {}


// baudrate manipulation 
BOOL CCommPort::SetBaudrate   ( DWORD dwBaudRate )
{
	if ( !IsOpen() )
	{
		return FALSE;
	}

	DCB dcb;
	BOOL result = GetCommState( m_hComm, &dcb );

	if ( result )
	{
		dcb.BaudRate = dwBaudRate;

		result = SetCommState( m_hComm, &dcb );
	}
	else
	{
		return FALSE; 
	}

	return result;
}

DWORD CCommPort::GetBaudrate ( )
{
	if ( !IsOpen() )
	{
		return 0;
	}

	DCB dcb;
	if ( GetCommState( m_hComm, &dcb ) )
        return dcb.BaudRate;
	else
		return 0;
}

// databits manipulation  
BOOL CCommPort::SetDataBits ( BYTE bDataBits )
{
	if ( !IsOpen() )
	{
		return FALSE;
	}

	DCB dcb;
	BOOL result = GetCommState( m_hComm, &dcb );

	if ( result )
	{
		dcb.ByteSize = bDataBits;

		result = SetCommState( m_hComm, &dcb );
	}
	else
	{
		return FALSE; 
	}

	return result;
}

BYTE CCommPort::GetDataBits ( )
{
	if ( !IsOpen() )
	{
		return 0;
	}

	DCB dcb;
	if( GetCommState( m_hComm, &dcb ) )
	{
		return dcb.ByteSize;
	}
	else 
	{
		return 0;
	}
}
// parity manipulation 
BOOL CCommPort::SetParity ( BYTE bParity )
{
	if ( !IsOpen() )
	{
		return FALSE;
	}

	DCB dcb;
	BOOL result = GetCommState( m_hComm, &dcb );

	if ( result )
	{
		dcb.Parity = bParity;

		result = SetCommState( m_hComm, &dcb );
	}
	else
	{
		return FALSE; 
	}

	return result;
}

BOOL CCommPort::GetParity ( BYTE& Parity )
{
	if ( !IsOpen() )
	{
		return FALSE; 
	}

	DCB dcb;
	if( GetCommState( m_hComm, &dcb ) )
	{
		Parity = dcb.Parity;
		return TRUE;
	}
	else 
	{
		return TRUE;
	}
}

BOOL CCommPort::SetStopBits ( BYTE bStopBits )
{
	if ( !IsOpen() )
	{
		return FALSE;
	}

	DCB dcb;
	BOOL result = GetCommState( m_hComm, &dcb );

	if ( result )
	{
		dcb.StopBits = bStopBits;

		result = SetCommState( m_hComm, &dcb );
	}
	else
	{
		return FALSE; 
	}

	return result;
}

BOOL CCommPort::GetStopBits ( BYTE &StopBits )
{
	if ( !IsOpen() )
	{
		return FALSE;	
	}

	DCB dcb;
	if( GetCommState( m_hComm, &dcb ) )
	{
		StopBits = dcb.StopBits;
		return TRUE;
	}
	else 
	{
		return FALSE;
	}
}

BOOL CCommPort::SetFlowControl( BYTE fc )
{
	if ( !IsOpen() )
	{
		return FALSE;	
	}

   DCB dcb;
   if( GetCommState( m_hComm, &dcb ) )
   {
		switch ( fc )
		{
			case NoFlowControl: 
				{
					dcb.fInX  = FALSE;
					dcb.fOutX = FALSE;
					dcb.fOutxDsrFlow = FALSE;
					dcb.fOutxCtsFlow = FALSE;
					dcb.fDtrControl = DTR_CONTROL_ENABLE;
					dcb.fRtsControl = RTS_CONTROL_ENABLE;
				} break;
			
			case XonXoffFlowControl:
				{
					dcb.fInX  = TRUE;
					dcb.fOutX = TRUE;
					dcb.fOutxDsrFlow = FALSE;
					dcb.fOutxCtsFlow = FALSE;
					dcb.fDtrControl = DTR_CONTROL_ENABLE;
					dcb.fRtsControl = RTS_CONTROL_ENABLE;
				} break;
			
			case Hardware:
				{
					dcb.fInX  = FALSE;
					dcb.fOutX = FALSE;
					dcb.fOutxDsrFlow  = TRUE;
					dcb.fOutxCtsFlow  = TRUE;
					dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
					dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
				} break;
		}
		return SetCommState( m_hComm, &dcb );
    }
    else
	{
		return FALSE;
	}
}