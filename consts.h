
#pragma once 

const TCHAR VAL_BAUDRATE [][7] = { 
					_T("100"), 
					_T("300"),
					_T("600"),
					_T("1200"),
					_T("2400"),
					_T("4800"),
					_T("9600"),
					_T("14400"),
					_T("19200"),
					_T("38400"),
					_T("56000"),
					_T("57600"),
					_T("115200"),
					_T("128000"),
					_T("256000") }; 
const int NUM_BAUDRATE = 15;

const TCHAR VAL_DATABITS [][2] = {
						_T("5"),
						_T("6"),
						_T("7"),
						_T("8")  };
const int NUM_DATABITS = 4;

const TCHAR VAL_PARITY [][6] = {
					_T("None"),
					_T("Odd"),
					_T("Even"), 
		            _T("Mark"),
					_T("Space") };
const int NUM_PARITY = 5;

const TCHAR VAL_STOPBIT [3][4] = { _T("1"), _T("1,5"), _T("2") };
const int NUM_STOPBIT = 3;


const TCHAR VAL_FC [][10] = {  _T("None"), _T("Xon\\Xoff"), _T("Hardware") };
const int  NUM_FC = 3;