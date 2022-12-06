#ifndef PIPE_FDS_H
#define PIPE_FDS_H
class PipeFds
{
public:
    PipeFds();
    ~PipeFds();

public:
    bool Init();

    // 子线程获取读数据的管道fd
    inline int GetPipeThreadReadFd() { return m_fds_master[0]; }
    // 子线程获取写数据的管道fd
    inline int GetPipeThreadWriteFd() { return m_fds_thread[1]; }
    // 主线程读数据的管道fd
    inline int GetPipeMasterReadFd() { return m_fds_thread[0]; }
    // 主线程写数据的管道fd
    inline int GetPipeMasterWriteFd() { return m_fds_master[1]; }

    void ClosePipeFds(int fds[]);

private:
    int m_fds_master[2];
    int m_fds_thread[2];
};
#endif // PIPE_FDS_H