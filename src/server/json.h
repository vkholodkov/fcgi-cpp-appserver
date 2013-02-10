
#ifndef _JSON_H_
#define _JSON_H_

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>

class JSONObject;
class JSONArray;

typedef enum {
    JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_INTEGER, JSON_FLOAT
} json_element_type_t;

struct JSONValue {

    JSONValue(JSONObject *_object)
    {
        type = JSON_OBJECT;
        d.object = _object;
    }

    JSONValue(JSONArray *_array)
    {
        type = JSON_ARRAY;
        d.array = _array;
    }

    JSONValue(int _i)
    {
        type = JSON_INTEGER;
        d.i = _i;
    }

    JSONValue(double _d)
    {
        type = JSON_FLOAT;
        d.d = _d;
    }

    JSONValue(const std::string &_str)
    {
        type = JSON_STRING;

        d.str = new char[sizeof(size_t) + _str.size()];

        if(d.str == NULL) {
            throw std::runtime_error("not enough memory");
        }

        size_t *len = reinterpret_cast<size_t*>(d.str);
        char *value = reinterpret_cast<char*>(len+1);

        *len = _str.size();
        std::copy(_str.begin(), _str.end(), value);
    }

    json_element_type_t type;

    union {
        JSONObject*     object;
        JSONArray*      array;
        int             i;
        double          d;
        char            *str;
    } d;
};

class JSONObject {
public:
    JSONObject()
        : m_items()
    {}

    ~JSONObject();

    void add(const std::string &key, JSONObject *value) {
        m_items.insert(std::make_pair(key, value));
    }

    void add(const std::string &key, JSONArray *value) {
        m_items.insert(std::make_pair(key, value));
    }

    void add(const std::string &key, const std::string &value) {
        m_items.insert(std::make_pair(key, value));
    }

    void add(const std::string &key, int value) {
        m_items.insert(std::make_pair(key, value));
    }

    void add(const std::string &key, double value) {
        m_items.insert(std::make_pair(key, value));
    }

    JSONArray *add_array(const std::string &key);
    JSONObject *add_object(const std::string &key);

private:
    friend std::ostream& operator<<(std::ostream& o, const JSONObject &object);
    std::map<std::string, struct JSONValue> m_items;
};

class JSONArray {
public:
    JSONArray()
        : m_items()
    {}

    ~JSONArray();

    void add(JSONObject *value) {
        m_items.push_back(value);
    }

    void add(JSONArray *value) {
        m_items.push_back(value);
    }

    void add(const std::string &value) {
        m_items.push_back(value);
    }

    void add(int value) {
        m_items.push_back(value);
    }

    void add(double value) {
        m_items.push_back(value);
    }

    JSONArray *add_array();
    JSONObject *add_object();

private:
    friend std::ostream& operator<<(std::ostream& o, const JSONArray &array);
    std::vector<struct JSONValue> m_items;
};

std::ostream& operator<<(std::ostream& o, const JSONObject &object);
std::ostream& operator<<(std::ostream& o, const JSONObject *object);
std::ostream& operator<<(std::ostream& o, const JSONArray &array);
std::ostream& operator<<(std::ostream& o, const JSONArray *array);

/*
class JSONParser {
public:
    JSONParser()
    {}

    JSONObject* parse(std::istream&);
};
*/

#endif //_JSON_H_
