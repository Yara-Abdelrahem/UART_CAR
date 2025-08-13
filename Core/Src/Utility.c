// #include <stdint.h>   // for uint8_t
// #include <stddef.h>   // for size_t
// #include <stdbool.h>  // for bool, true, false
// #include <string.h>   // for strlen, memcpy, etc.
// #include <stdio.h>    // for snprintf, printf

// int UTIL_atoi(const char *snum)
// {
//   if (snum[0] == '\0')
//     return -1;

//   int number = -1;
//   int i = 0;
//   for (i = 0; snum[i]; i++)
//   {
//     if ((snum[i] >= '0') && (snum[i] <= '9'))
//     {
//       if (number == -1)
//       {
//         number = 0;
//       }

//       number = number * 10;
//       number += (snum[i] - '0');
//     }
//     else
//     {
//       return -1;
//     }
//   }
//   CP_Utility_LOG("string converted to number: %d", number);
//   return number;
// }


// void UTIL_convertStrToHexByte(char str[], uint8_t hex_byte[]) {
//     hex_byte[0] = 0x0;
//     hex_byte[0] |= (str[0] - '0') & 0x0F;

//     for (int i = 1; i < 8; i++) {
//         hex_byte[i] = ((str[(2 * i) - 1] << 4) & 0xF0);
//         hex_byte[i] |= (str[(2 * i)] & 0x0F);
//     }

  
// }

// void convert_to_hex_string(const char *buf, size_t len, char *hex_str) {

//     hex_str[0] = '\0';
//     for (size_t i = 0; i < len; i++) {
//         snprintf(hex_str + strlen(hex_str), 3, "%02X", (unsigned char)buf[i]);
//     }
// }



// char charToUpper(char c)
// {
// 	char t = c;
// 	if (('a' <= c) && (c <= 'z'))
// 	{
// 		t = 'A' + (c - 'a');
// 	}
// 	return t;
// }
// void strToUpper(char s[])
// {
// 	CP_Utility_LOG("str before toUpper: [%s]", s);

// 	for (int i = 0; s[i]; i++)
// 	{
// 		s[i] = charToUpper(s[i]);
// 	}
// 	CP_Utility_LOG("str after toUpper: [%s]", s);
// }

// char charToLower(char c)
// {
// 	char t = c;
// 	if (('A' <= c) && (c <= 'Z'))
// 	{
// 		t = 'a' + (c - 'A');
// 	}
// 	return t;
// }
// void strToLower(char s[])
// {
// 	CP_Utility_LOG("str before toLower: [%s]", s);


//      size_t len = strlen(s);
//     for (size_t i = 0; i < len; i++)

// 	{

// 		//s[i] = 'a';
// 		s[i] = charToLower(s[i]);
// 	}
// 	CP_Utility_LOG("str after toLower: [%s]", s);
// }

// bool isStrStartsWith(char str[], char subStr[])
// {
// 	int len1;
// 	int len2;
// 	int minLen;
// 	int i;

// 	len1 = strlen(str);
// 	len2 = strlen(subStr);

// 	minLen = len1 > len2 ? len2 : len1;

// 	for (i = 0; i < minLen; i++)
// 	{
// 		if (str[i] != subStr[i])
// 		{
// 			break;
// 		}
// 	}
// 	if (i == minLen)
// 	{
// 		return true;
// 	}
// 	else
// 	{
// 		return false;
// 	}
// }

// bool isCmdEndsWithHash(char cmd[])
// {
// 	int cmdLen = strlen(cmd);
// 	if ((cmdLen >= 1) && (cmd[cmdLen - 1] == '#'))
// 	{
// 		return true;
// 	}
// 	return false;
// }
// void deleteEndHash(char cmd[])
// {
// 	int len = strlen(cmd);
// 	CP_Utility_LOG("len %d", len);
// 	CP_Utility_LOG("delete last hash [%c]", cmd[len - 1]);
// 	cmd[len - 1] = '\0';
// }