#ifndef __VERSION_COMMON_H__
#define __VERSION_COMMON_H__
#define	__VER	15
#define __MAINSERVER
#if !defined( __TESTSERVER ) && !defined( __MAINSERVER )
	#define __INTERNALSERVER
#endif

#define		__SERVER				// 클라이언트 전용코드를 빌드하지 않기 위한 define
#define		__DOS1101
#define		__CRC
#define		__SO1014				// 소켓 예외 처리( 캐쉬, 인증, 로그인 )
#define		__PROTOCOL0910
#define		__PROTOCOL1021
#define		__VERIFYNETLIB
#define		__DOS1101
#define		__STL_0402		// stl

#if (_MSC_VER > 1200)
#define		__VS2003
#if (_MSC_VER >= 1600) // __VC100
#pragma warning ( disable : 4995 )
#pragma warning ( disable : 4996 )
#endif // __VC100
#endif

// 15차
//	#define		__2ND_PASSWORD_SYSTEM			// 로그인 시 2차 비밀번호 입력

#if	  defined(__INTERNALSERVER)	// 내부 사무실 테스트서버 


#elif defined(__TESTSERVER)		// 외부 유저 테스트서버


#elif defined(__MAINSERVER)		// 외부 본섭

#endif	// end - 서버종류별 define 


#endif

