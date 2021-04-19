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


typedef struct jsonValue
{
	int type;
	const char *ptr;
} jsonValue;


jsonValue jsonParse(const char *str, int len);

int jsonBoolean(jsonValue j);
int jsonInteger(jsonValue j);
double jsonDouble(jsonValue j);
int jsonString(jsonValue j, char *value, int max);
int jsonObjectKey(jsonValue j, char *value, int max);
jsonValue jsonObjectValue(jsonValue j);
jsonValue jsonObjectValueByKey(jsonValue j, const char *key);
jsonValue jsonNext(jsonValue j);


#endif
