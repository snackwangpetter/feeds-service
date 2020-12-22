#ifndef _FEEDS_AUTOUPDATE_HPP_
#define _FEEDS_AUTOUPDATE_HPP_

#include <functional>
#include <memory>
#include <StdFileSystem.hpp>

namespace trinity {

class ThreadPool;

class AutoUpdate {
public:
    /*** type define ***/

    /*** static function and variable ***/
    static std::shared_ptr<AutoUpdate> GetInstance();

    /*** class function and variable ***/
    int needUpdate(int64_t newVerCode);
    int asyncDownloadTarball(const std::string& url,
                             const std::filesystem::path& cacheDir,
                             const std::string &name,
                             int64_t size,
                             const std::string md5,
                             const std::function<void(int errCode)>& resultCallback);

protected:
    /*** type define ***/

    /*** static function and variable ***/

    /*** class function and variable ***/

private:
    /*** type define ***/

    /*** static function and variable ***/
    static std::shared_ptr<AutoUpdate> AutoUpdateInstance;

    /*** class function and variable ***/
    explicit AutoUpdate() = default;
    virtual ~AutoUpdate() = default;

    int downloadTarball(const std::string &url,
                        const std::filesystem::path& cacheDir,
                        const std::string &name,
                        int64_t size,
                        const std::string md5,
                        const std::function<void(int errCode)> &resultCallback);
    int checkTarball(const std::string &filepath, int64_t size, const std::string md5);

    std::shared_ptr<ThreadPool> threadPool;
};

/***********************************************/
/***** class template function implement *******/
/***********************************************/

/***********************************************/
/***** macro definition ************************/
/***********************************************/

} // namespace trinity

#endif /* _FEEDS_AUTOUPDATE_HPP_ */


