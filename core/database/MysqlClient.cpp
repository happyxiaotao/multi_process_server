
#include "MysqlClient.h"
#include "../log/Log.h"

#include <mutex>
static std::mutex g_mutex;

SDbResult::~SDbResult()
{
    Clear();
}
void SDbResult::Clear()
{
    m_naffected = 0;

    if (m_title != nullptr)
    {
        for (size_t i = 0; i < m_nfield; i++)
        {
            delete[] m_title[i];
        }
        delete[] m_title;
    }
    m_title = nullptr;

    if (m_data != nullptr)
    {
        for (size_t i = 0; i < m_nrow; i++)
        {
            for (size_t j = 0; j < m_nfield; j++)
            {
                delete[] m_data[i][j];
            }
            delete[] m_data[i];
        }
        delete[] m_data;
        m_data = nullptr;
    }

    m_nfield = 0;
    m_nrow = 0;
}
void SDbResult::Dump()
{
    std::string str;
    str.reserve(2048);
    for (size_t i = 0; i < m_nfield; i++)
    {
        str.append("|");
        str.append(m_title[i]);
    }
    str.append("|\n");

    for (size_t i = 0; i < m_nrow; i++)
    {
        for (size_t j = 0; j < m_nfield; j++)
        {
            str.append("|");
            str.append(m_data[i][j]);
        }
        str.append("|\n");
    }
    Trace("------------------------ Dump mysql result begin -----------------------\n"
          "{}"
          "------------------------ Dump mysql result end   -----------------------\n",
          str);
}

void CMysqlClient::InitMysqliClientLibrary()
{
    mysql_library_init(0, nullptr, nullptr);
}
void CMysqlClient::ReleaseMysqlClientLibrary()
{
    mysql_library_end();
}

// 关于mysql的线程调用api，可以参考:https://www.mysqlzh.com/doc/196/123.html
CMysqlClient::CMysqlClient() : m_bIsOpen(false)
{
    mysql_thread_init(); //可由my_init()和mysql_connect()自动调用。
}
CMysqlClient::~CMysqlClient()
{
    Close();
    mysql_thread_end(); // 该函数不会被其他函数自动调用，需要手动调用，避免内存泄漏
}

bool CMysqlClient::Open(const std::string &ip, u_short port,
                        const std::string &user, const std::string &passport,
                        const std::string &dbname,
                        size_t connect_timeout, size_t read_timeout, size_t write_timeout)
{
    Close();

    MYSQL *mysql = mysql_init(&m_mysql); // mysql_init非线程安全的

    if (mysql == nullptr)
    {
        Error("mysql_init failed, Unable to allocate a MYSQL object!");
        return false;
    }

    m_bIsOpen = true;

    if (connect_timeout != 0)
        mysql_options(&m_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout);
    if (read_timeout != 0)
        mysql_options(&m_mysql, MYSQL_OPT_READ_TIMEOUT, &read_timeout);
    if (write_timeout != 0)
        mysql_options(&m_mysql, MYSQL_OPT_WRITE_TIMEOUT, &write_timeout);
    if (mysql_real_connect(&m_mysql, ip.c_str(), user.c_str(), passport.c_str(), dbname.c_str(), port, nullptr, 0) == nullptr)
    {
        Error("mysql_read_connect failed, ip:{},port:{},user:{},dbname:{},error:{}", ip, port, user, dbname, mysql_error(&m_mysql));
        Close();
        return false;
    }

    int reconnect = 1;
    mysql_options(&m_mysql, MYSQL_OPT_RECONNECT, &reconnect); // 支持重连,放在mysql_real_connect之后
    // mysql_set_character_set(&m_mysql, "utf8");                // 使用utf8编码
    mysql_set_character_set(&m_mysql, "utf8mb4"); // 使用utf8mb4编码

    return true;
}

void CMysqlClient::Close()
{
    if (m_bIsOpen)
    {
        mysql_close(&m_mysql);
        m_bIsOpen = false;
    }
}
char *CMysqlClient::EscapeString(const char *value, size_t len, size_t *out_len)
{
    *out_len = 0;
    char *new_value = new char[len * 2 + 1];
    u_long n = mysql_real_escape_string(&m_mysql, new_value, value, len);
    *out_len = n;
    return new_value;
}
std::string CMysqlClient::EscapeString(const std::string &strValue)
{
    size_t out_len;
    char *out = EscapeString(strValue.c_str(), strValue.size(), &out_len);
    std::string strEscapeString(out, out_len);
    delete[] out;
    return std::move(strEscapeString);
}
SDbResult *CMysqlClient::Query(const char *sql, size_t len)
{
    if (!IsOpen())
    {
        std::string strSql(sql, len);
        Error("Mysql还未连接成功，无法查询数据，sql:{}", strSql);
        return nullptr;
    }
    int ret = mysql_real_query(&m_mysql, sql, len);
    if (ret != 0)
    {
        Error("mysql_real_query failed, new_sql:{}, errcode:{}, error:{}", sql, mysql_errno(&m_mysql), mysql_error(&m_mysql));
        return nullptr;
    }

    // mysql官网说明:https://www.mysqlzh.com/api/66.html
    // 查询（SELECT、SHOW、DESCRIBE、EXPLAIN、CHECK TABLE等 才会有数据返回结果集
    // 其他情况（INSERT、UPDATE、DELETE等）返回nullptr,当错误发生时也是返回nullptr,可以通过错误码确认是空数据集还是有错误发生
    MYSQL_RES *res = mysql_store_result(&m_mysql);
    if (res == nullptr && mysql_errno(&m_mysql))
    {
        Error("mysql_store_result failed, error:{}", mysql_error(&m_mysql));
        return nullptr;
    }
    // 非查询类会返回nullptr
    if (res == nullptr)
    {
        return nullptr;
    }
    SDbResult *result = GetResData(res);
    mysql_free_result(res); // mysql_store_result调用的返回的结果集，需要通过mysql_free_result来释放资源

    return result;
}
int CMysqlClient::Exec(const char *sql, size_t len)
{
    if (!IsOpen())
    {
        std::string strSql(sql, len);
        Error("Mysql还未连接成功，无法处理数据，sql:{}", strSql);
        return -1;
    }
    int ret = mysql_real_query(&m_mysql, sql, len);
    if (ret != 0)
    {
        Error("mysql_real_query failed, new_sql:{}, errcode:{}, error:{}", sql, mysql_errno(&m_mysql), mysql_error(&m_mysql));
        return -2;
    }
    // 参考：https://www.mysqlzh.com/api/1.html
    // 返回上次UPDATE更改的行数，上次DELETE删除的行数，或上次INSERT语句插入的行数。
    // 对于UPDATE、DELETE或INSERT语句，可在mysql_query()后立刻调用。
    // 对于SELECT语句，mysql_affected_rows()的工作方式与mysql_num_rows()类似。
    int nAffected = mysql_affected_rows(&m_mysql);
    Trace("mysql sql exec success,sql:{}, affected:{}", sql, nAffected);
    return nAffected;
}

SDbResult *CMysqlClient::GetResData(MYSQL_RES *res)
{
    // mysql_affected_rows的说明参考官网：https://www.mysqlzh.com/api/1.html
    my_ulonglong naffected = mysql_affected_rows(&m_mysql); //执行INSERT、UPDATE或DELETE时，影响的行数
    u_int nrow = mysql_num_rows(res);                       // 返回行的个数
    u_int nfield = mysql_num_fields(res);                   // 获取字段个数

    SDbResult *result = new SDbResult();
    result->m_naffected = naffected;
    result->m_nrow = nrow;
    result->m_nfield = nfield;

    // 拷贝表头
    result->m_title = new char *[nfield];
    u_int name_length = 0;
    MYSQL_FIELD *fields = mysql_fetch_fields(res);
    for (u_int i = 0; i < nfield; i++)
    {
        name_length = fields[i].name_length;
        result->m_title[i] = new char[name_length + 1];
        memcpy(result->m_title[i], fields[i].name, name_length);
        result->m_title[i][name_length] = '\0';
    }

    // 拷贝数据集
    result->m_data = new char **[nrow];
    for (u_int i = 0; i < nrow; i++)
    {
        result->m_data[i] = new char *[nfield];
        for (uint j = 0; j < nfield; j++)
        {
            result->m_data[i][j] = nullptr;
        }
    }

    MYSQL_ROW row = nullptr;
    u_int pos_row = 0;
    u_long value_length = 0;
    while ((row = mysql_fetch_row(res)) != nullptr)
    {
        u_long *length = mysql_fetch_lengths(res);
        for (u_int j = 0; j < nfield; j++)
        {
            value_length = length[j];
            result->m_data[pos_row][j] = new char[value_length + 1];
            memcpy(result->m_data[pos_row][j], row[j], value_length);
            result->m_data[pos_row][j][value_length] = '\0';
        }
        pos_row++;
    }

    // 只有调用了mysql_fetch_row()获取了所有行之后，才能调用mysql_num_rows()
    return result;
}

bool CMysqlClient::DeleteTableRows(const char *pTable, const char *pKey, const char *pValue, size_t value_len)
{
    if (pTable == nullptr || pKey == nullptr || pValue == nullptr || value_len == 0)
    {
        return false;
    }
    size_t out_len;
    char *out = EscapeString(pValue, value_len, &out_len);

    std::string strSql;
    strSql.reserve(128);

    strSql.append("delete from `").append(pTable).append("` where `").append(pKey).append("`='").append(out, out_len).append("'");
    delete[] out;
    int nAffected = Exec(strSql);
    if (nAffected < 0)
    {
        Error("CMysqlClient::DeleteTableRows, 执行sql语句失败，sql:{}", strSql);
        return false;
    }
    return true;
}

bool CMysqlClient::Ping()
{
    return mysql_ping(&m_mysql) == 0; // =0表示成功，其他值表示异常
}
