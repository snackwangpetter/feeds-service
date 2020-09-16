#include "MassDataProcessor.hpp"

#include <crystal.h>
#include <ela_carrier.h>
#include <ela_session.h>
#include <functional>
#include <SafePtr.hpp>
extern "C" {
#define new fix_cpp_keyword_new
#include <auth.h>
#include <did.h>
#undef new
}

namespace elastos {

/* =========================================== */
/* === static variables initialize =========== */
/* =========================================== */

/* =========================================== */
/* === static function implement ============= */
/* =========================================== */
#if __cpp_lib_shared_ptr_arrays < 201611
template <class T, class U>
static std::shared_ptr<T> reinterpret_pointer_cast(const std::shared_ptr<U> &r) noexcept {
    auto p = reinterpret_cast<typename std::shared_ptr<T>::element_type *>(r.get());
    return std::shared_ptr<T>(r, p);
}
#endif

/* =========================================== */
/* === class public function implement  ====== */
/* =========================================== */
MassDataProcessor::MassDataProcessor()
{
    using namespace std::placeholders;
    mothodHandleMap = {
        {Method::SetBinary, std::bind(&MassDataProcessor::onSetBinary, this, _1, _2, _3)},
        {Method::GetBinary, std::bind(&MassDataProcessor::onGetBinary, this, _1, _2, _3)},
    };
}

MassDataProcessor::~MassDataProcessor()
{
}

void MassDataProcessor::config(const std::filesystem::path& massDataDir)
{
    this->massDataDir = massDataDir;
}

int MassDataProcessor::dispose(const std::vector<uint8_t>& headData,
                               const std::filesystem::path& bodyPath)
{
    Log::D(Log::TAG, "%s", __PRETTY_FUNCTION__);

    Req *reqBuf = nullptr;
    int ret = rpc_unmarshal_req(headData.data(), headData.size(), &reqBuf);
    auto deleter = [=](Req* ptr) -> void {
        vlogD("Destroy req buffer.");
        deref(ptr);
    };
    auto req = std::shared_ptr<Req>(reqBuf, deleter); // workaround: declear for auto release req pointer
    if (ret == -1) {
        CHECK_ERROR(ErrCode::MassDataUnmarshalReqFailed);
    } else if (ret == -2) {
        CHECK_ERROR(ErrCode::MassDataUnknownReqFailed);
    }
    auto resp = std::shared_ptr<Resp>();

    Log::D(Log::TAG, "Mass data processor: dispose method [%s]", req->method);
    ret = ErrCode::UnimplementedError;
    for(const auto& it: mothodHandleMap) {
        if(std::strcmp(it.first, req->method) == 0) {
            ret = it.second(req, bodyPath, resp);
            break;
        }
    }
    CHECK_ERROR(ret);

    Marshalled* marshalData = rpc_marshal_resp(req->method, resp.get());
    CHECK_ASSERT(marshalData, ErrCode::MassDataMarshalRespFailed);

    auto marshalDataPtr = reinterpret_cast<uint8_t*>(marshalData->data);
    resultHeadData = {marshalDataPtr, marshalDataPtr + marshalData->sz};

    return 0;
}

int MassDataProcessor::getResult(std::vector<uint8_t>& headData,
                                 std::filesystem::path& bodyPath)
{
    headData = std::move(resultHeadData);
    bodyPath = resultBodyPath;

    return 0;
}

/* =========================================== */
/* === class protected function implement  === */
/* =========================================== */


/* =========================================== */
/* === class private function implement  ===== */
/* =========================================== */
int MassDataProcessor::isOwner(const std::string& accessToken)
{
    if (did_is_ready() == false) {
        Log::E(Log::TAG, "Mass data processor: Feeds DID is not ready.");
        CHECK_ERROR(ErrCode::DidNotReady);
    }

    UserInfo* uinfo = create_uinfo_from_access_token(accessToken.c_str());
    if (uinfo == nullptr) {
        Log::E(Log::TAG, "Mass data processor: Invalid access token.");
        CHECK_ERROR(ErrCode::InvalidAccessToken);
    }

    if (user_id_is_owner(uinfo->uid) == false) {
        Log::E(Log::TAG, "Mass data processor: Set binary while not being owner.");
        CHECK_ERROR(ErrCode::NotAuthorizedError);
    }

    return 0;
}

int MassDataProcessor::onSetBinary(std::shared_ptr<Req> req,
                                   const std::filesystem::path& bodyPath,
                                   std::shared_ptr<Resp>& resp)
{
    auto setBinReq = reinterpret_pointer_cast<SetBinaryReq>(req);
    auto setBinResp = std::make_shared<SetBinaryResp>();
    setBinResp->tsx_id = setBinReq->tsx_id;
    setBinResp->result.key = setBinReq->params.key;

    Log::D(Log::TAG, "    access_token: %s", setBinReq->params.tk);
    Log::D(Log::TAG, "    key: %s", setBinReq->params.key);
    Log::D(Log::TAG, "    algo: %s", setBinReq->params.algo);
    Log::D(Log::TAG, "    checksum: %s", setBinReq->params.checksum);

    // TODO: For Test
    // int ret = isOwner(setBinReq->params.tk);
    // CHECK_ERROR(ret);

    if(std::strcmp(setBinReq->params.algo, "None") != 0) {
        setBinResp->result.errcode = ErrCode::MassDataUnsupportedAlgo;
        CHECK_ERROR(ErrCode::MassDataUnsupportedAlgo);
    }

    auto keyPath = massDataDir / setBinReq->params.key;
    Log::V(Log::TAG, "Resave %s to %s.", bodyPath.c_str(), keyPath.c_str());
    std::filesystem::rename(bodyPath, keyPath);

    resp = reinterpret_pointer_cast<Resp>(setBinResp);

    return 0;
}

int MassDataProcessor::onGetBinary(std::shared_ptr<Req> head,
                                   const std::filesystem::path& bodyPath,
                                   std::shared_ptr<Resp>& resp)
{
    Log::D(Log::TAG, "%s", __PRETTY_FUNCTION__);

    return -1;
}

} // namespace elastos