#ifndef _MYSQL_CLIENT_H_
#define _MYSQL_CLIENT_H_

#include <mysql/mysql.h>
#include <string>
#include <vector>

struct SDbResult
{
    SDbResult() : m_naffected(0), m_nrow(0), m_nfield(0), m_title(nullptr), m_data(nullptr)
    {
    }
    ~SDbResult();

    void Clear();

    void Dump();

    my_ulonglong m_naffected; // 上次查询影响的行数。返回值参考：https://www.mysqlzh.com/api/1.html
    u_long m_nrow;            // 数据集-行数
    u_long m_nfield;          // 数据集-列数
    char **m_title;           // 数据集-表头
    char ***m_data;           // 保存列表数据
};

class CMysqlClient
{
public:
    CMysqlClient();
    virtual ~CMysqlClient();

public:
    static void InitMysqliClientLibrary();
    static void ReleaseMysqlClientLibrary();

public:
    bool Open(const std::string &ip, u_short port,
              const std::string &user, const std::string &passport,
              const std::string &dbname,
              size_t connect_timeout = 0, size_t read_timeout = 0, size_t write_timeout = 0);

    void Close();

    char *EscapeString(const char *value, size_t len, size_t *out_len);

    std::string EscapeString(const std::string &strValue);

    // 返回查询类的结果，会返回一个结果集
    SDbResult *Query(const char *sql, size_t len);
    SDbResult *Query(const std::string &strSql) { return Query(strSql.c_str(), strSql.size()); }

    // 返回非查询类的结果（INSERT UPDATE DELETE等）。
    // 返回<0表示失败，返回>=0表示生效行数
    int Exec(const char *sql, size_t len);
    int Exec(const std::string &strSql) { return Exec(strSql.c_str(), strSql.size()); }

    // 执行ping
    bool Ping();

    inline bool IsOpen() { return m_bIsOpen; }

    // 只适合结果是string的，不适用于interage和float
    bool DeleteTableRows(const char *pTable, const char *key, const char *value, size_t value_len);

private:
    SDbResult *GetResData(MYSQL_RES *res);

private:
    bool m_bIsOpen;
    MYSQL m_mysql;
};

#endif //_MYSQL_CLIENT_H_
