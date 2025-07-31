//
// Created by lxz on 2025/7/29.
//


#include "zJson.h"

// 构造函数
zJson::zJson(const string& json_str) : type_(Type::NULL_VALUE), value_(nullptr), json_str_(json_str), is_error_(true) {
    LOGD("[zJson] Constructor called with json_str: %s", json_str.c_str());
    if (!json_str.empty()) {
        parse(json_str);
    }
}

// 创建无效的JSON对象
zJson zJson::createInvalid() {
    LOGD("[zJson] createInvalid called");
    zJson json;
    json.type_ = Type::INVALID;
    json.is_error_ = true;
    return json;
}

// 拷贝构造函数
zJson::zJson(const zJson& other) : type_(other.type_), value_(other.value_), json_str_(other.json_str_), is_error_(other.is_error_) {
    LOGD("[zJson] Copy constructor called");
}

// 赋值运算符
zJson& zJson::operator=(const zJson& other) {
    LOGD("[zJson] operator= called");
    if (this != &other) {
        type_ = other.type_;
        value_ = other.value_;
        json_str_ = other.json_str_;
        is_error_ = other.is_error_;
    }
    return *this;
}

// 解析JSON字符串
void zJson::parse(const string& json_str) {
    LOGD("[zJson] parse called with json_str: %s", json_str.c_str());
    try {
        size_t pos = 0;
        skipWhitespace(json_str, pos);
        parseValue(json_str, pos);
        
        // 检查解析是否成功
        if (is_error_) {
            LOGW("[zJson] JSON parsing failed for: %s", json_str.c_str());
        } else {
            LOGI("[zJson] JSON parsing successful for: %s", json_str.c_str());
        }
    } catch (...) {
        // 解析失败时设置为错误状态
        LOGW("[zJson] JSON parsing exception occurred");
        type_ = Type::INVALID;
    }
}

// 解析JSON值
void zJson::parseValue(const string& json_str, size_t& pos) {
    LOGD("[zJson] parseValue called at pos: %zu", pos);
    skipWhitespace(json_str, pos);
    
    if (pos >= json_str.length()) {
        LOGE("[zJson] JSON parsing failed: Unexpected end of JSON string at position %zu", pos);
        type_ = Type::INVALID;
        return;
    }
    
    char c = json_str[pos];
    LOGD("[zJson] JSON parsing: current char '%c' at position %zu", c, pos);
    
    if (c == '{') {
        parseObject(json_str, pos);
    } else if (c == '[') {
        parseArray(json_str, pos);
    } else if (c == '"') {
        type_ = Type::STRING;
        value_ = parseString(json_str, pos);
    } else if (c == 't' || c == 'f') {
        type_ = Type::BOOLEAN;
        value_ = parseBoolean(json_str, pos);
    } else if (c == 'n') {
        parseNull(json_str, pos);
    } else if (c == '-' || (c >= '0' && c <= '9')) {
        type_ = Type::NUMBER;
        value_ = parseNumber(json_str, pos);
    } else {
        LOGE("[zJson] JSON parsing failed: Unexpected character '%c' at position %zu", c, pos);
        type_ = Type::INVALID;
    }
}

// 解析对象
void zJson::parseObject(const string& json_str, size_t& pos) {
    LOGD("[zJson] parseObject called at pos: %zu", pos);
    type_ = Type::OBJECT;
    map<string, zJson> obj;
    
    pos++; // 跳过 '{'
    skipWhitespace(json_str, pos);
    
    if (pos >= json_str.length()) {
        LOGE("[zJson] JSON parsing failed: Unexpected end of JSON string in object at position %zu", pos);
        type_ = Type::INVALID;
        return;
    }
    
    // 检查是否为空对象
    if (json_str[pos] == '}') {
        pos++;
        value_ = obj;
        return;
    }
    
    while (pos < json_str.length()) {
        // 解析键
        if (json_str[pos] != '"') {
            LOGE("[zJson] JSON parsing failed: Expected '\"' for object key at position %zu", pos);
            type_ = Type::INVALID;
            return;
        }
        
        string key = parseString(json_str, pos);
        if (is_error_) {
            LOGE("[zJson] JSON parsing failed: Failed to parse object key");
            return;
        }
        
        skipWhitespace(json_str, pos);
        
        // 检查冒号
        if (pos >= json_str.length() || json_str[pos] != ':') {
            LOGE("[zJson] JSON parsing failed: Expected ':' after object key at position %zu", pos);
            type_ = Type::INVALID;
            return;
        }
        pos++;
        
        skipWhitespace(json_str, pos);
        
        // 解析值
        zJson value;
        value.parseValue(json_str, pos);
        if (value.isError()) {
            LOGE("[zJson] JSON parsing failed: Failed to parse object value");
            type_ = Type::INVALID;
            return;
        }
        
        obj[key] = value;
        
        skipWhitespace(json_str, pos);
        
        if (pos >= json_str.length()) {
            LOGE("[zJson] JSON parsing failed: Unexpected end of JSON string in object at position %zu", pos);
            type_ = Type::INVALID;
            return;
        }
        
        if (json_str[pos] == '}') {
            pos++;
            break;
        } else if (json_str[pos] == ',') {
            pos++;
            skipWhitespace(json_str, pos);
        } else {
            LOGE("[zJson] JSON parsing failed: Expected ',' or '}' in object at position %zu", pos);
            type_ = Type::INVALID;
            return;
        }
    }
    
    value_ = obj;
}

// 解析数组
void zJson::parseArray(const string& json_str, size_t& pos) {
    LOGD("[zJson] JSON parsing: parsing array at position %zu", pos);
    type_ = Type::ARRAY;
    vector<zJson> arr;
    
    pos++; // 跳过 '['
    skipWhitespace(json_str, pos);
    
    if (pos >= json_str.length()) {
        LOGE("[zJson] JSON parsing failed: Unexpected end of JSON string in array at position %zu", pos);
        type_ = Type::INVALID;
        return;
    }
    
    if (json_str[pos] == ']') {
        pos++; // 跳过 ']'
        value_ = arr;
        is_error_ = false; // 解析成功
        LOGD("[zJson] JSON parsing: empty array parsed successfully");
        return;
    }
    
    while (pos < json_str.length()) {
        skipWhitespace(json_str, pos);
        
        zJson element;
        element.parseValue(json_str, pos);
        arr.push_back(element);
        
        skipWhitespace(json_str, pos);
        
        if (pos >= json_str.length()) {
            LOGE("[zJson] JSON parsing failed: Unexpected end of JSON string in array at position %zu", pos);
            type_ = Type::INVALID;
            return;
        }
        
        if (json_str[pos] == ']') {
            pos++; // 跳过 ']'
            is_error_ = false; // 解析成功
            LOGD("[zJson] JSON parsing: array parsed successfully with %zu elements", arr.size());
            break;
        } else if (json_str[pos] == ',') {
            pos++; // 跳过 ','
        } else {
            LOGE("[zJson] JSON parsing failed: Expected ',' or ']' in array, got '%c' at position %zu", json_str[pos], pos);
            type_ = Type::INVALID;
            return;
        }
    }
    
    value_ = arr;
    is_error_ = false; // 解析成功
}

// 解析字符串
string zJson::parseString(const string& json_str, size_t& pos) {
    LOGD("[zJson] JSON parsing: parsing string at position %zu", pos);
    pos++; // 跳过开始的引号
    string result;
    
    while (pos < json_str.length()) {
        char c = json_str[pos++];
        
        if (c == '"') {
            is_error_ = false; // 解析成功
            LOGD("[zJson] JSON parsing: string parsed successfully: '%s'", result.c_str());
            return result; // 直接返回，不进行反转义处理
        } else if (c == '\\') {
            if (pos >= json_str.length()) {
                LOGE("[zJson] JSON parsing failed: Unexpected end of JSON string in string at position %zu", pos);
                type_ = Type::INVALID;
                return "";
            }
            char next = json_str[pos++];
            LOGD("[zJson] JSON parsing: escape sequence '\\%c'", next);
            switch (next) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    if (pos + 4 > json_str.length()) {
                        LOGE("[zJson] JSON parsing failed: Invalid unicode escape sequence at position %zu", pos);
                        type_ = Type::INVALID;
                        return "";
                    }
                    // 简化处理，直接跳过4个字符
                    pos += 4;
                    result += '?'; // 占位符
                    break;
                }
                default:
                    // 对于未知的转义字符，直接保留原字符
                    LOGD("[zJson] JSON parsing: unknown escape sequence '\\%c', keeping as-is", next);
                    result += next;
                    break;
            }
        } else {
            result += c;
        }
    }
    
    LOGE("[zJson] JSON parsing failed: Unterminated string");
    type_ = Type::INVALID;
    return "";
}

// 解析数字
double zJson::parseNumber(const string& json_str, size_t& pos) {
    LOGD("[zJson] JSON parsing: parsing number at position %zu", pos);
    size_t start = pos;
    
    if (json_str[pos] == '-') {
        pos++;
    }
    
    while (pos < json_str.length() && isdigit(json_str[pos])) {
        pos++;
    }
    
    if (pos < json_str.length() && json_str[pos] == '.') {
        pos++;
        while (pos < json_str.length() && isdigit(json_str[pos])) {
            pos++;
        }
    }
    
    if (pos < json_str.length() && (json_str[pos] == 'e' || json_str[pos] == 'E')) {
        pos++;
        if (pos < json_str.length() && (json_str[pos] == '+' || json_str[pos] == '-')) {
            pos++;
        }
        while (pos < json_str.length() && isdigit(json_str[pos])) {
            pos++;
        }
    }
    
    string num_str = json_str.substr(start, pos - start);
    LOGD("[zJson] JSON parsing: number string '%s'", num_str.c_str());
    char* endptr;
    double result = strtod(num_str.c_str(), &endptr);
    
    if (*endptr != '\0') {
        LOGE("[zJson] JSON parsing failed: Invalid number format '%s'", num_str.c_str());
        type_ = Type::INVALID;
        return 0.0;
    }
    
    is_error_ = false; // 解析成功
    LOGD("[zJson] JSON parsing: number parsed successfully: %f", result);
    return result;
}

// 解析布尔值
bool zJson::parseBoolean(const string& json_str, size_t& pos) {
    LOGD("[zJson] JSON parsing: parsing boolean at position %zu", pos);
    if (json_str.substr(pos, 4) == "true") {
        pos += 4;
        is_error_ = false; // 解析成功
        LOGD("[zJson] JSON parsing: boolean parsed successfully: true");
        return true;
    } else if (json_str.substr(pos, 5) == "false") {
        pos += 5;
        is_error_ = false; // 解析成功
        LOGD("[zJson] JSON parsing: boolean parsed successfully: false");
        return false;
    } else {
        LOGE("[zJson] JSON parsing failed: Invalid boolean value at position %zu", pos);
        type_ = Type::INVALID;
        return false;
    }
}

// 解析null
void zJson::parseNull(const string& json_str, size_t& pos) {
    LOGD("[zJson] JSON parsing: parsing null at position %zu", pos);
    if (json_str.substr(pos, 4) == "null") {
        pos += 4;
        type_ = Type::NULL_VALUE;
        value_ = nullptr;
        is_error_ = false; // 解析成功
        LOGD("[zJson] JSON parsing: null parsed successfully");
    } else {
        LOGE("[zJson] JSON parsing failed: Invalid null value at position %zu", pos);
        type_ = Type::INVALID;
    }
}

// 跳过空白字符
void zJson::skipWhitespace(const string& json_str, size_t& pos) {
    while (pos < json_str.length() && isspace(json_str[pos])) {
        pos++;
    }
}

// 直接获取对象中的布尔值
bool zJson::getBoolean(const string& key, bool defaultValue) const {
    if (type_ != Type::OBJECT) {
        return defaultValue;
    }
    const auto& obj = std::get<map<string, zJson>>(value_);
    auto it = obj.find(key);
    if (it == obj.end()) {
        return defaultValue;
    }
    return it->second.getBooleanValue(defaultValue);
}

// 直接获取对象中的双精度浮点数
double zJson::getDouble(const string& key, double defaultValue) const {
    if (type_ != Type::OBJECT) {
        return defaultValue;
    }
    const auto& obj = std::get<map<string, zJson>>(value_);
    auto it = obj.find(key);
    if (it == obj.end()) {
        return defaultValue;
    }
    return it->second.getDoubleValue(defaultValue);
}

// 直接获取对象中的整数
int zJson::getInt(const string& key, int defaultValue) const {
    if (type_ != Type::OBJECT) {
        return defaultValue;
    }
    const auto& obj = std::get<map<string, zJson>>(value_);
    auto it = obj.find(key);
    if (it == obj.end()) {
        return defaultValue;
    }
    return it->second.getIntValue(defaultValue);
}

// 直接获取对象中的长整数
long zJson::getLong(const string& key, long defaultValue) const {
    LOGD("[zJson] getLong called for key: '%s', defaultValue: %ld", key.c_str(), defaultValue);
    
    if (type_ != Type::OBJECT) {
        LOGD("[zJson] getLong failed: not an object, type: %d", (int)type_);
        return defaultValue;
    }
    const auto& obj = std::get<map<string, zJson>>(value_);
    auto it = obj.find(key);
    if (it == obj.end()) {
        LOGD("[zJson] getLong failed: key '%s' not found", key.c_str());
        return defaultValue;
    }
    
    LOGD("[zJson] getLong: found key '%s', calling getLongValue", key.c_str());
    long result = it->second.getLongValue(defaultValue);
    LOGD("[zJson] getLong: result: %ld", result);
    return result;
}

// 直接获取对象中的字符串
string zJson::getString(const string& key, const string& defaultValue) const {
    if (type_ != Type::OBJECT) {
        return defaultValue;
    }
    const auto& obj = std::get<map<string, zJson>>(value_);
    auto it = obj.find(key);
    if (it == obj.end()) {
        return defaultValue;
    }
    return it->second.getStringValue(defaultValue);
}

// 获取JSON对象
zJson zJson::getJSONObject(const string& key, const zJson& defaultValue) const {
    if (type_ != Type::OBJECT) {
        return defaultValue;
    }
    const auto& obj = std::get<map<string, zJson>>(value_);
    auto it = obj.find(key);
    if (it == obj.end()) {
        return defaultValue;
    }
    if (!it->second.isObject()) {
        return defaultValue;
    }
    return it->second;
}

// 获取JSON数组
zJson zJson::getJSONArray(const string& key, const zJson& defaultValue) const {
    if (type_ != Type::OBJECT) {
        return defaultValue;
    }
    const auto& obj = std::get<map<string, zJson>>(value_);
    auto it = obj.find(key);
    if (it == obj.end()) {
        return defaultValue;
    }
    if (!it->second.isArray()) {
        return defaultValue;
    }
    return it->second;
}

// 获取当前对象的布尔值
bool zJson::getBooleanValue(bool defaultValue) const {
    if (type_ != Type::BOOLEAN) {
        return defaultValue;
    }
    return std::get<bool>(value_);
}

// 获取当前对象的双精度浮点数
double zJson::getDoubleValue(double defaultValue) const {
    if (type_ != Type::NUMBER) {
        return defaultValue;
    }
    return std::get<double>(value_);
}

// 获取当前对象的整数
int zJson::getIntValue(int defaultValue) const {
    if (type_ != Type::NUMBER) {
        return defaultValue;
    }
    return static_cast<int>(std::get<double>(value_));
}

// 获取当前对象的长整数
long zJson::getLongValue(long defaultValue) const {
    LOGD("[zJson] getLongValue called, type: %d, defaultValue: %ld", (int)type_, defaultValue);
    
    if (type_ != Type::NUMBER) {
        LOGD("[zJson] getLongValue failed: not a number, type: %d", (int)type_);
        return defaultValue;
    }
    
    double doubleValue = std::get<double>(value_);
    long result = static_cast<long>(doubleValue);
    LOGD("[zJson] getLongValue: double value: %f, long result: %ld", doubleValue, result);
    return result;
}

// 获取当前对象的字符串
string zJson::getStringValue(const string& defaultValue) const {
    if (type_ != Type::STRING) {
        return defaultValue;
    }
    return std::get<string>(value_);
}

// 获取数组元素
zJson zJson::get(int index, const zJson& defaultValue) const {
    if (type_ != Type::ARRAY) {
        return defaultValue;
    }
    const auto& arr = std::get<vector<zJson>>(value_);
    if (index < 0 || static_cast<size_t>(index) >= arr.size()) {
        return defaultValue;
    }
    return arr[index];
}

// 获取数组长度
int zJson::length() const {
    if (type_ != Type::ARRAY) {
        throw std::runtime_error("Value is not an array");
    }
    return static_cast<int>(std::get<vector<zJson>>(value_).size());
}

// 检查对象是否包含键
bool zJson::has(const string& key) const {
    if (type_ != Type::OBJECT) {
        return false;
    }
    const auto& obj = std::get<map<string, zJson>>(value_);
    return obj.find(key) != obj.end();
}

// 获取对象属性
zJson zJson::get(const string& key, const zJson& defaultValue) const {
    if (type_ != Type::OBJECT) {
        return defaultValue;
    }
    const auto& obj = std::get<map<string, zJson>>(value_);
    auto it = obj.find(key);
    if (it == obj.end()) {
        return defaultValue;
    }
    return it->second;
}

// 获取对象的所有键
vector<string> zJson::keys() const {
    if (type_ != Type::OBJECT) {
        throw std::runtime_error("Value is not an object");
    }
    const auto& obj = std::get<map<string, zJson>>(value_);
    vector<string> result;
    for (const auto& pair : obj) {
        result.push_back(pair.first);
    }
    return result;
}

// 转换为字符串
string zJson::toString() const {
    if (is_error_) {
        return "null";
    }
    
    switch (type_) {
        case Type::NULL_VALUE:
            return "null";
        case Type::BOOLEAN:
            return std::get<bool>(value_) ? "true" : "false";
        case Type::NUMBER: {
            double num = std::get<double>(value_);
            if (num == static_cast<int>(num)) {
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "%f", num);
                string result(buffer);
            } else {
                // 使用 sprintf 格式化浮点数
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "%.6f", num);
                string result(buffer);
                
                // 移除尾部的0
                while (result.back() == '0' && result.find('.') != string::npos) {
                    result.pop_back();
                }
                if (result.back() == '.') {
                    result.pop_back();
                }
                return result;
            }
        }
        case Type::STRING:
            return "\"" + escapeString(std::get<string>(value_)) + "\"";
        case Type::ARRAY: {
            const auto& arr = std::get<vector<zJson>>(value_);
            string result = "[";
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) result += ",";
                result += arr[i].toString();
            }
            result += "]";
            return result;
        }
        case Type::OBJECT: {
            const auto& obj = std::get<map<string, zJson>>(value_);
            string result = "{";
            bool first = true;
            for (const auto& pair : obj) {
                if (!first) result += ",";
                result += "\"" + escapeString(pair.first) + "\":" + pair.second.toString();
                first = false;
            }
            result += "}";
            return result;
        }
        case Type::INVALID:
        default:
            return "null";
    }
}

// 转义字符串
string zJson::escapeString(const string& str) const {
    string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '/': result += "\\/"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: 
                // 对于其他字符，直接保留
                result += c; 
                break;
        }
    }
    return result;
}

// 反转义字符串
string zJson::unescapeString(const string& str) const {
    string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"': result += '"'; i++; break;
                case '\\': result += '\\'; i++; break;
                case '/': result += '/'; i++; break;
                case 'b': result += '\b'; i++; break;
                case 'f': result += '\f'; i++; break;
                case 'n': result += '\n'; i++; break;
                case 'r': result += '\r'; i++; break;
                case 't': result += '\t'; i++; break;
                default: result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}


