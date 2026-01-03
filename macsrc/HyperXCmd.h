/************************************/
/* File:HyperXCmd.h  */
/* */
/* Interface for standard */ 
/* HyperCard callback routines.    */
/* */
/* Based on original work by*/
/* Dan Winkler of Apple Computer */
/* */
/************************************/

/*
typedef struct Str31 {
 char data[32];
 } Str31;
typedef  Str31 * Str31Ptr;
*/

typedef struct XCmdBlock {
 short  paramCount;       
    Handle  params[16];
    Handle  returnValue;      
    Boolean passFlag; 
    
    void  (*entryPoint)();    
    short request;  
    short result;  
    long inArgs[8];
    long outArgs[4];
} XCmdBlock;
typedef XCmdBlock*XCmdBlockPtr; 

  /* Callback codes  */
#define xresSucc 0
#define xresFail 1 
#define xresNotImp 2 
  
  /* Callback request codes */
#define xreqSendCardMessage 1 
#define xreqEvalExpr 2 
#define xreqStringLength  3 
#define xreqStringMatch   4 

#define xreqZeroBytes         6 
#define xreqPasToZero 7 
#define xreqZeroToPas 8 
#define xreqStrToLong 9 
#define xreqStrToNum 10 
#define xreqStrToBool 11 
#define xreqStrToExt 12 
#define xreqLongToStr 13 
#define xreqNumToStr 14 
#define xreqNumToHex 15 
#define xreqBoolToStr 16 
#define xreqExtToStr 17 
#define xreqGetGlobal 18 
#define xreqSetGlobal 19 
#define xreqGetFieldByName 20 
#define xreqGetFieldByNum 21 
#define xreqGetFieldByID  22 
#define xreqSetFieldByName 23 
#define xreqSetFieldByNum 24 
#define xreqSetFieldByID  25 
#define xreqStringEqual       26 
#define xreqReturnToPas       27 
#define xreqScanToReturn      28 
#define xreqScanToZero        39  

/* 
 ,ÄúPrototypes,Äù for the Callbacks.  Project 
 must include XCmdGlue.c.  
*/

 
pascal void SendCardMessage();     
pascal Handle  EvalExpr();
pascal long StringLength(); 
pascal Ptr  StringMatch();
pascal void ZeroBytes();
pascal Handle  PasToZero();
pascal void ZeroToPas();
pascal long StrToLong();
pascal long StrToNum();
pascal Boolean   StrToBool();
pascal void StrToExt();
pascal void LongToStr();
pascal void NumToStr();
pascal void NumToHex();
pascal void BoolToStr();
pascal void ExtToStr();
pascal Handle  GetGlobal();
pascal void SetGlobal();
pascal Handle  GetFieldByName();
pascal Handle  GetFieldByNum();
pascal Handle  GetFieldByID();
pascal void SetFieldByName();
pascal void SetFieldByNum();
pascal void SetFieldByID();
pascal Boolean   StringEqual();
pascal void ReturnToPas();
pascal void ScanToReturn();
pascal void ScanToZero();