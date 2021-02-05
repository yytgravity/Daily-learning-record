最近学习了单元测试来测试漏洞，在这里做个简单的记录：

参考： https://source.chromium.org/chromium/chromium/src/+/19aeffd4d93f5b82cf6877ff83614ff9cb6e1d1f


目标函数：
```
void NetworkContext::VerifyCertForSignedExchange(
    const scoped_refptr<net::X509Certificate>& certificate,
    const GURL& url,
    const net::NetworkIsolationKey& network_isolation_key,
    const std::string& ocsp_result,
    const std::string& sct_list,
    VerifyCertForSignedExchangeCallback callback) {
  if (require_network_isolation_key_)
    DCHECK(!network_isolation_key.IsEmpty());

  int cert_verify_id = ++next_cert_verify_id_;
  auto pending_cert_verify = std::make_unique<PendingCertVerify>();
  pending_cert_verify->callback = std::move(callback);
  pending_cert_verify->result = std::make_unique<net::CertVerifyResult>();
  pending_cert_verify->certificate = certificate;
  pending_cert_verify->url = url;
  pending_cert_verify->network_isolation_key = network_isolation_key;
  pending_cert_verify->ocsp_result = ocsp_result;
  pending_cert_verify->sct_list = sct_list;
  net::CertVerifier* cert_verifier =
      g_cert_verifier_for_testing ? g_cert_verifier_for_testing
                                  : url_request_context_->cert_verifier();
  int result = cert_verifier->Verify(
      net::CertVerifier::RequestParams(certificate, url.host(),
                                       0 /* cert_verify_flags */, ocsp_result,
                                       sct_list),
      pending_cert_verify->result.get(),
      base::BindOnce(&NetworkContext::OnVerifyCertForSignedExchangeComplete,
                     base::Unretained(this), cert_verify_id),
      &pending_cert_verify->request,
      net::NetLogWithSource::Make(url_request_context_->net_log(),
                                  net::NetLogSourceType::CERT_VERIFIER_JOB));
  cert_verifier_requests_[cert_verify_id] = std::move(pending_cert_verify);

  if (result != net::ERR_IO_PENDING)
    OnVerifyCertForSignedExchangeComplete(cert_verify_id, result);
}
```

其实也可以通过mojo来调用，但是我太菜了（驾驭不了。。），所以这里选择了单元测试。
首先我们在当前模块里找前人写过的测试代码，然后进行自己的魔改，这样的话比较省劲。

![](./img/1.png)
这里选择了ssl_config_service_mojo_unittest.cc作为目标，但是这里和参考还是有些区别的：

```
source_set("tests") {
  testonly = true

  sources = [
    "chunked_data_pipe_upload_data_stream_unittest.cc",
    "cookie_manager_unittest.cc",
    "cookie_settings_unittest.cc",
    "cors/cors_url_loader_factory_unittest.cc",
    "cors/cors_url_loader_unittest.cc",
    ....
    ]
```
他这里是使用了source_set("tests")。
这种的编译方法：

1、gn ls out/release > log可以打印出所有的target，之后在target中找我们选中的模块，就找到了services/network:tests
2、在代码中搜services/network:tests，就可以找到unittest。
![](./img/2.png)
3、ninja -C out/release services_unittests
4、./services_unittests --gtest_filter=xx.xx
这里的xx就是你自己要测试的那个单元。

这是本次测试的单元，所以--gtest_filter=NetworkServiceSSLConfigServiceTest.NoSSLConfig
```
TEST_F(NetworkServiceSSLConfigServiceTest, NoSSLConfig) {
```

简单说几个学到的知识：
用到的变量常量函数等，要在相应的类中申明,比如本例中就是NetworkServiceSSLConfigServiceTest，当然相应的头文件要自己去include：
```
class NetworkServiceSSLConfigServiceTest : public testing::Test {
```

剩下就是写测试时候的细节问题了，这些就等写的过程中在发现吧。（ps 单元测试调试很方便，遇到问题不如gdb一下）。