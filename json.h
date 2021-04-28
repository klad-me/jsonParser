#ifndef JSON_H
#define JSON_H


enum
{
	JSON_NULL,
	JSON_FALSE,
	JSON_TRUE,
	JSON_NUMBER,
	JSON_STRING,
	JSON_ARRAY,
	JSON_OBJECT,
};


typedef char(*json_getc)(void*, int pos);


typedef struct jsonPtr
{
	json_getc	cb;
	void*		cb_param;
	int			pos;
} jsonPtr;


typedef struct jsonValue
{
	char		type;
	jsonPtr		ptr;
} jsonValue;


jsonValue jsonParse(json_getc cb, void *param);
jsonValue jsonParseString(const char *str);

bool jsonBoolean(const jsonValue &j);
int jsonInteger(const jsonValue &j);
double jsonDouble(const jsonValue &j);
int jsonString(const jsonValue &j, char *value, int max);
int jsonObjectKey(const jsonValue &j, char *value, int max);
jsonValue jsonObjectValue(const jsonValue &j);
jsonValue jsonObjectValueByKey(const jsonValue &j, const char *key);
jsonValue jsonNext(const jsonValue &j);


#endif
