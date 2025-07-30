//
// Created by lxz on 2025/7/29.
//

#ifndef OVERT_APP_SRC_MAIN_CPP_ZJSON_H_
#define OVERT_APP_SRC_MAIN_CPP_ZJSON_H_


#include <variant>

#include "zLog.h"
#include "config.h"


class zJson {
public:
    // JSON值类型
    enum class Type {
        NULL_VALUE,
        BOOLEAN,
        NUMBER,
        STRING,
        ARRAY,
        OBJECT,
        INVALID  // 新增：表示解析失败或无效状态
    };

    // JSON值存储
    using JsonValue = std::variant<std::nullptr_t, bool, double, string, vector<zJson>, map<string, zJson>>;
    
    // 解析状态
    bool is_error_;

private:
    Type type_;
    JsonValue value_;
    string json_str_; // 原始JSON字符串

public:
    // 构造函数 - 接受JSON字符串
    zJson(const string& json_str = "");
    
    // 静态方法：创建无效的JSON对象
    static zJson createInvalid();
    
    // 拷贝构造函数
    zJson(const zJson& other);
    
    // 赋值运算符
    zJson& operator=(const zJson& other);
    
    // 析构函数
    ~zJson() = default;

    // 类型检查方法
    bool isNull() const { return type_ == Type::NULL_VALUE; }
    bool isBoolean() const { return type_ == Type::BOOLEAN; }
    bool isNumber() const { return type_ == Type::NUMBER; }
    bool isString() const { return type_ == Type::STRING; }
    bool isArray() const { return type_ == Type::ARRAY; }
    bool isObject() const { return type_ == Type::OBJECT; }

    // 对象操作 - 直接获取不同类型的值
    bool has(const string& key) const;
    zJson get(const string& key, const zJson& defaultValue = zJson()) const;
    
    // 直接获取不同类型的值（仿Java JSONObject风格）
    bool getBoolean(const string& key, bool defaultValue = false) const;
    double getDouble(const string& key, double defaultValue = 0.0) const;
    int getInt(const string& key, int defaultValue = 0) const;
    long getLong(const string& key, long defaultValue = 0L) const;
    string getString(const string& key, const string& defaultValue = "") const;
    zJson getJSONObject(const string& key, const zJson& defaultValue = zJson()) const;
    zJson getJSONArray(const string& key, const zJson& defaultValue = zJson()) const;
    
    // 数组操作
    zJson get(int index, const zJson& defaultValue = zJson()) const;
    int length() const;
    
    // 获取所有键
    vector<string> keys() const;
    

    
    // 直接获取当前对象的值（用于已获取的JSON对象）
    bool getBooleanValue(bool defaultValue = false) const;
    double getDoubleValue(double defaultValue = 0.0) const;
    int getIntValue(int defaultValue = 0) const;
    long getLongValue(long defaultValue = 0L) const;
    string getStringValue(const string& defaultValue = "") const;
    
    // 获取类型
    Type getType() const { return type_; }
    
    // 转换为字符串
    string toString() const;
    
    // 获取原始JSON字符串
    string getJsonString() const { return json_str_; }
    
    // 检查JSON对象是否有效
    bool isError() const { return is_error_; }

private:
    // 解析方法
    void parse(const string& json_str);
    void parseValue(const string& json_str, size_t& pos);
    void parseObject(const string& json_str, size_t& pos);
    void parseArray(const string& json_str, size_t& pos);
    string parseString(const string& json_str, size_t& pos);
    double parseNumber(const string& json_str, size_t& pos);
    bool parseBoolean(const string& json_str, size_t& pos);
    void parseNull(const string& json_str, size_t& pos);
    
    // 辅助方法
    void skipWhitespace(const string& json_str, size_t& pos);
    string escapeString(const string& str) const;
    string unescapeString(const string& str) const;
};

#endif //OVERT_APP_SRC_MAIN_CPP_ZJSON_H_
