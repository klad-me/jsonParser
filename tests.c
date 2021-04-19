#include <stdio.h>
#include "json.h"


static void printArray(jsonValue j, int indent);
static void printObject(jsonValue j, int indent);


static void printValue(jsonValue j, int indent)
{
	char str[100];
	
	//printf("value ptr='%s'\n", j.ptr);
	
	switch (j.type)
	{
		case JSON_NULL:
			printf("%*snull\n", indent, "");
			break;
		
		case JSON_FALSE:
			printf("%*sfalse\n", indent, "");
			break;
		
		case JSON_TRUE:
			printf("%*sfalse\n", indent, "");
			break;
		
		case JSON_NUMBER:
			printf("%*s%.6f\n", indent, "", jsonDouble(j));
			break;
		
		case JSON_STRING:
			jsonString(j, str, sizeof(str));
			printf("%*s\"%s\"\n", indent, "", str);
			break;
		
		case JSON_ARRAY:
			printf("%*sArray:\n", indent, "");
			printArray(j, indent+2);
			break;
		
		case JSON_OBJECT:
			printf("%*sObject:\n", indent, "");
			printObject(j, indent+2);
			break;
	}
}


static void printArray(jsonValue j, int indent)
{
	int i=0;
	//printf("array ptr='%s'\n", j.ptr);
	while (j.type == JSON_ARRAY)
	{
		printf("%*s[%d]\n", indent+2, "", i);
		printValue(jsonObjectValue(j), indent+4);
		
		j=jsonNext(j);
	}
}


static void printObject(jsonValue j, int indent)
{
	char key[100];
	//printf("object ptr='%s'\n", j.ptr);
	while (j.type == JSON_OBJECT)
	{
		jsonObjectKey(j, key, sizeof(key));
		printf("%*sKey '%s'\n", indent+2, "", key);
		printValue(jsonObjectValue(j), indent+4);
		
		j=jsonNext(j);
	}
}


int main()
{
	const char* types[]=
	{
		"null",
		"false",
		"true",
		"number",
		"string",
		"array",
		"object"
	};
	
	char json[65536];
	
#if 1
	FILE *f=fopen("test.json", "r");
	if (! f)
	{
		perror("test.json");
		return -1;
	}
	int len=fread(json, 1, sizeof(json), f);
	fclose(f);
#else
	strcpy(json, "{\"hello\": {\"wow\": 1, \"bau\": 2}, \"world\": [1,2,3]}");
	int len=strlen(json);
#endif
	
	
	
	jsonValue j=jsonParse(json, len);
	printf("result=%s\n", types[j.type]);
	printObject(j, 0);
	
	return 0;
}
