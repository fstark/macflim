/************************************/
/* File: XCMDGlue.c*/
/* */
/* Callback routines for XCMDs*/
/* and XFCNs.  This file should  */
/* be included in your project*/
/* */
/* Based on original work by*/
/* Dan Winkler of Apple Computer */
/* */
/************************************/

#include <MacHeaders>
#include "HyperXCmd.h"
#include<math.h>

pascal void SendCardMessage(paramPtr,msg)
 XCmdBlockPtr  paramPtr;  
 StringPtr msg;
/***********************
* Send a message back to 
* hypercard.  The input message
* is a Pascal String
***********************/
{
 paramPtr->inArgs[0] = (long)msg;
 paramPtr->request = xreqSendCardMessage;
    CallPascal( paramPtr->entryPoint );
}

pascal Handle EvalExpr(paramPtr,expr)
 XCmdBlockPtr  paramPtr;  
 StringPtr expr;
/***********************
* Evaluate a Hypertalk expression
* returning the result as a ,ÄúC,Äù 
* string
***********************/
{
 paramPtr->inArgs[0] = (long)expr;
 paramPtr->request = xreqEvalExpr;
    CallPascal( paramPtr->entryPoint );
 return (Handle)paramPtr->outArgs[0];
}

pascal long StringLength(paramPtr,strPtr)
 XCmdBlockPtr  paramPtr;
 StringPtr strPtr;
/***********************
* Counts the number of 
* characters in the input 
* string from StrPtr to end
* of string (zero byte)
***********************/
{
 paramPtr->inArgs[0] = (long)strPtr;
 paramPtr->request = xreqStringLength;
    CallPascal( paramPtr->entryPoint );
 return (long)paramPtr->outArgs[0];
}

pascal Ptr StringMatch(paramPtr,pattern,target)
 XCmdBlockPtr  paramPtr;  
 StringPtr pattern;
 Ptr    target;
/***********************
* Case-insensitive match 
* for pattern anywhere in
* target, 
* 
* Returns a pointer to first
* character of the first match,
* in target or NIL if no match
* found.  pattern is a Pascal string,
* and target is a zero-terminated string.
***********************/
{
 paramPtr->inArgs[0] = (long)pattern;
 paramPtr->inArgs[1] = (long)target;
 paramPtr->request = xreqStringMatch;
    CallPascal( paramPtr->entryPoint );
 return (Ptr)paramPtr->outArgs[0];
}

pascal void ZeroBytes(paramPtr,dstPtr,longCount)
 XCmdBlockPtr  paramPtr;  
 Ptr    dstPtr;  
 long   longCount;
/***********************
* Clear memory starting at destPtr
* through destPtr+longCount
***********************/
{
 paramPtr->inArgs[0] = (long)dstPtr;
 paramPtr->inArgs[1] = longCount;
 paramPtr->request = xreqZeroBytes;
    CallPascal( paramPtr->entryPoint );
}

pascal Handle PasToZero(paramPtr,pasStr)
 XCmdBlockPtr  paramPtr;
 StringPtr pasStr;
/***********************
* Convert a Pascal string (STR255)
* to a zero-terminated string.  
* Returns a handle to a zero-terminated
* string.  The caller must dispose the handle.
*
* Useful for setting the result or
* an argument you send from 
* an XCMD to HyperTalk.
***********************/
{
 paramPtr->inArgs[0] = (long)pasStr;
 paramPtr->request = xreqPasToZero;
    CallPascal( paramPtr->entryPoint );
 return (Handle)paramPtr->outArgs[0];
}

pascal void ZeroToPas(paramPtr,zeroStr,pasStr)
 XCmdBlockPtr  paramPtr;  
 char   *zeroStr;
 StringPtr pasStr;
/***********************
* Copy the zero-terminated
* string into the Pascal String.
*
* You create the Pascal string 
* and pass it by reference.
*
***********************/
{
 paramPtr->inArgs[0] = (long)zeroStr;
 paramPtr->inArgs[1] = (long)pasStr;
 paramPtr->request = xreqZeroToPas;
    CallPascal( paramPtr->entryPoint );
}

pascal long StrToLong(paramPtr,strPtr)
 XCmdBlockPtr  paramPtr;  
 Str31  *strPtr;
/***********************
* Convert a string of ASCII
* characters to an unsigned 
* long integer.
***********************/
{
 paramPtr->inArgs[0] = (long)strPtr;
 paramPtr->request = xreqStrToLong;
    CallPascal( paramPtr->entryPoint );
 return (long)paramPtr->outArgs[0];
}

pascal long StrToNum(paramPtr,str)
 XCmdBlockPtr  paramPtr;  
 Str31  *str;
/***********************
* Convert a string of ASCII
* characters to a signed 
* long integer.
***********************/
{
 paramPtr->inArgs[0] = (long)str;
 paramPtr->request = xreqStrToNum;
    CallPascal( paramPtr->entryPoint );
 return paramPtr->outArgs[0];
}

pascal Boolean StrToBool(paramPtr,str)
 XCmdBlockPtr  paramPtr;
 Str31  *str;
/***********************
* Convert the Pascal strings
* ,Äòtrue,Äô and ,Äòfalse,Äô to booleans.
***********************/
{
 paramPtr->inArgs[0] = (long)str;
 paramPtr->request = xreqStrToBool;
    CallPascal( paramPtr->entryPoint );
 return (Boolean)paramPtr->outArgs[0];
}

pascal void StrToExt(paramPtr,str,myext)
 XCmdBlockPtr  paramPtr;
 Str31  *str;  
 long   *myext;
/***********************
* Convert a string of ASCII digits 
* to an extended long integer.
*
* The return value is passed
* by reference and you must
* asllocate the space before 
* calling this routine.
***********************/
{
 paramPtr->inArgs[0] = (long)str;
 paramPtr->inArgs[1] = (long)myext;
 paramPtr->request = xreqStrToExt;
    CallPascal( paramPtr->entryPoint );
}

pascal void LongToStr(paramPtr,posNum,mystr)
 XCmdBlockPtr  paramPtr;  
 long   posNum;  
 Str31  *mystr;
/***********************
* Convert an unsigned long integer
* to a pascal string representation
* Useful for sending numbers back 
* to Hypercard.
*
*  You create mystr and pass
* it by reference.
***********************/
{
 paramPtr->inArgs[0] = (long)posNum;
 paramPtr->inArgs[1] = (long)mystr;
 paramPtr->request = xreqLongToStr;
    CallPascal( paramPtr->entryPoint );
}

pascal void NumToStr(paramPtr,num,mystr)
 XCmdBlockPtr  paramPtr;  
 long   num;
 Str31  *mystr;
/***********************
* Convert a signed long integer
* to a pascal string representation
* Useful for sending numbers back 
* to Hypercard.
*
*  You create mystr and pass
* it by reference.
***********************/
{
 paramPtr->inArgs[0] = num;
 paramPtr->inArgs[1] = (long)mystr;
 paramPtr->request = xreqNumToStr;
    CallPascal( paramPtr->entryPoint );
}

pascal void NumToHex(paramPtr,num,nDigits,mystr)
 XCmdBlockPtr  paramPtr;  
 long   num;
 short  nDigits; 
 Str31  *mystr;
/***********************
* Convert an unsigned long integer
* to a hexadecimal number and put it
* into a Pascal string.
*
* The ,Äúoutput,Äù string is passed
* by reference.
***********************/
{
 paramPtr->inArgs[0] = num;
 paramPtr->inArgs[1] = nDigits;
 paramPtr->inArgs[2] = (long)mystr;
 paramPtr->request = xreqNumToHex;
    CallPascal( paramPtr->entryPoint );
}

pascal void BoolToStr(paramPtr,bool,mystr)
 XCmdBlockPtr  paramPtr;  
 Boolean bool;  
 Str31  *mystr;
/***********************
* Convert a boolean to 
* ,Äòtrue,Äô or ,Äòfalse,Äô.  
*
* The ,Äúoutput,Äù string is passed
* by reference.
***********************/
{
 paramPtr->inArgs[0] = (long)bool;
 paramPtr->inArgs[1] = (long)mystr;
 paramPtr->request = xreqBoolToStr;
    CallPascal( paramPtr->entryPoint );
}

pascal void ExtToStr( paramPtr, myext, mystr)
 XCmdBlockPtr  paramPtr;  
 char   *myext;  
 Str31  *mystr;
/***********************
* Convert an extended long
* to its string representation
*
* The ,Äúoutput,Äù string is passed
* by reference.
***********************/
{
 paramPtr->inArgs[0] = (long)myext;
 paramPtr->inArgs[1] = (long)mystr;
 paramPtr->request = xreqExtToStr;
    CallPascal( paramPtr->entryPoint );
}

pascal Handle GetGlobal(paramPtr,globName)
 XCmdBlockPtr  paramPtr;  
 StringPtr globName;
/***********************
* Return a handle to a zero-terminated
* string containing the value of 
* the specified HyperTalk global variable.
***********************/
{
 paramPtr->inArgs[0] = (long)globName;
 paramPtr->request = xreqGetGlobal;
    CallPascal( paramPtr->entryPoint );
 return (Handle)paramPtr->outArgs[0];
}

pascal void SetGlobal(paramPtr,globName,globValue)
 XCmdBlockPtr  paramPtr;  
 StringPtr globName;
 Handle globValue;
/***********************
* Set the value of the specified 
* HyperTalk global variable to be
* the zero-terminated string in globValue.
* The contents of globValue
* are copied, you dispose the
* handle
***********************/
{
 paramPtr->inArgs[0] = (long)globName;
 paramPtr->inArgs[1] = (long)globValue;
 paramPtr->request = xreqSetGlobal;
    CallPascal( paramPtr->entryPoint );
}

pascal Handle GetFieldByName(paramPtr,cardFieldFlag,fieldName)
 XCmdBlockPtr  paramPtr;  
 Boolean cardFieldFlag;
 StringPtr fieldName;
/***********************
* Return a handle to a zero-terminated
* string containing the value of 
* field fieldName on the current 
* card.  You must dispose the handle.
*
* Set cardfieldFlag to ture if
* you want the contents of a card
* field or to false if you want
* the contents of a bkgnd field
***********************/
{
 paramPtr->inArgs[0] = (long)cardFieldFlag;
 paramPtr->inArgs[1] = (long)fieldName;
 paramPtr->request = xreqGetFieldByName;
    CallPascal( paramPtr->entryPoint );
 return (Handle)paramPtr->outArgs[0];
}

pascal Handle GetFieldByNum(paramPtr,cardFieldFlag,fieldNum)
 XCmdBlockPtr  paramPtr;  
 Boolean cardFieldFlag;
 short  fieldNum;
/***********************
* Returns a copy of the contents of the field whose number is
* fieldnum on the current card.
* You dispose the handle when you are done.
***********************/
{
 paramPtr->inArgs[0] = (long)cardFieldFlag;
 paramPtr->inArgs[1] = fieldNum;
 paramPtr->request = xreqGetFieldByNum;
    CallPascal( paramPtr->entryPoint );
 return (Handle)paramPtr->outArgs[0];
}

pascal Handle GetFieldByID(paramPtr,cardFieldFlag,fieldID)
 XCmdBlockPtr  paramPtr;  
 Boolean cardFieldFlag;
 short  fieldID;
/***********************
* Returns a copy of the contents of the field whose id is
* fieldID on the current card.
* You dispose the handle when you are done.
***********************/
{
 paramPtr->inArgs[0] = (long)cardFieldFlag;
 paramPtr->inArgs[1] = fieldID;
 paramPtr->request = xreqGetFieldByID;
    CallPascal( paramPtr->entryPoint );
 return (Handle)paramPtr->outArgs[0];
}

pascal void SetFieldByName(paramPtr,cardFieldFlag,fieldName,fieldVal)
 XCmdBlockPtr  paramPtr;  
 Boolean cardFieldFlag;
 StringPtr fieldName; 
 Handle fieldVal;
/***********************
* Set the value of the field whose name is fieldName on 
* the current card.
* You dispose the handle when you are done.
***********************/
{
 paramPtr->inArgs[0] = (long)cardFieldFlag;
 paramPtr->inArgs[1] = (long)fieldName;
 paramPtr->inArgs[2] = (long)fieldVal;
 paramPtr->request = xreqSetFieldByName;
    CallPascal( paramPtr->entryPoint );
}

pascal void SetFieldByNum(paramPtr,cardFieldFlag,fieldNum,fieldVal)
 XCmdBlockPtr  paramPtr;  
 Boolean cardFieldFlag;
 short  fieldNum;
 Handle fieldVal;
/***********************
* Set the value of the field whose number is fieldnum on 
* the current card.
* You dispose the handle when you are done.
***********************/
{
 paramPtr->inArgs[0] = (long)cardFieldFlag;
 paramPtr->inArgs[1] = fieldNum;
 paramPtr->inArgs[2] = (long)fieldVal;
 paramPtr->request = xreqSetFieldByNum;
    CallPascal( paramPtr->entryPoint );
}

pascal void SetFieldByID(paramPtr,cardFieldFlag,fieldID,fieldVal)
 XCmdBlockPtr  paramPtr;  
 Boolean cardFieldFlag;
 short  fieldID; 
 Handle fieldVal;
/***********************
* Set the value of the field whose id is fieldID on 
* the current card.
* You dispose the handle when
* you are done.
***********************/
{
 paramPtr->inArgs[0] = (long)cardFieldFlag;
 paramPtr->inArgs[1] = fieldID;
 paramPtr->inArgs[2] = (long)fieldVal;
 paramPtr->request = xreqSetFieldByID;
    CallPascal( paramPtr->entryPoint );
}

pascal Boolean StringEqual(paramPtr,str1,str2)
 XCmdBlockPtr  paramPtr;  
 Str31  *str1; 
 Str31  *str2;
/***********************
* Returns true if the strings match, false otherwise.
* Compare is case insensitive
***********************/
{
 paramPtr->inArgs[0] = (long)str1;
 paramPtr->inArgs[1] = (long)str2;
 paramPtr->request = xreqStringEqual;
    CallPascal( paramPtr->entryPoint );
 return (Boolean)paramPtr->outArgs[0];
}

pascal void ReturnToPas(paramPtr,zeroStr,pasStr)
 XCmdBlockPtr  paramPtr;  
 Ptr    zeroStr; 
 StringPtr pasStr;
/***********************
* Collect characters from zeroStr
* to the next carriage Return and return 
* them in the Pascal string pasStr. 
* If no Return found, collect chars
* until the end of the string (zero)
***********************/
{
 paramPtr->inArgs[0] = (long)zeroStr;
 paramPtr->inArgs[1] = (long)pasStr;
 paramPtr->request = xreqReturnToPas;
    CallPascal( paramPtr->entryPoint );
}

pascal void ScanToReturn(paramPtr,scanHndl)
 XCmdBlockPtr  paramPtr;  
 Ptr    *scanHndl;
/***********************
* Position the pointer, scanPtr,at a Return character
* or a zero byte. 
***********************/
{
 paramPtr->inArgs[0] = (long)scanHndl;
 paramPtr->request = xreqScanToReturn;
    CallPascal( paramPtr->entryPoint );
}

pascal void ScanToZero(paramPtr,scanHndl)
 XCmdBlockPtr  paramPtr;  
 Ptr    *scanHndl;
/***********************
* Position the pointer, scanPtr,
* at a  zero byte. 
***********************/
{
 paramPtr->inArgs[0] = (long)scanHndl;
 paramPtr->request = xreqScanToZero;
    CallPascal( paramPtr->entryPoint );
}