# HTTPS读取验收

## 2026-10-04证书请求与HEAD响应

输入：Overt2.0.3／943，9464419字节，APK SHA-256为`8977b8002d0e32ecc62604acb5727951e3341fb94f8a59b6a0e0441fd65867ea`。真实Baidu GET返回超过64KiB正文，原生结果为`Response exceeds 64KiB limit`。定位结果为`美国纽约州纽约`、safe；`proc_info`为空对象，原页面双轮未完成。

步骤：登记Go1.26.8编排NDKr29／Clang和LLD21，Android15普通Guest UID2000、Enforcing。TCP、TLS及证书校验实际执行，只控制读取返回的响应记录；新增HEAD长度70000、分块头及分离响应头三项。

红灯：三项均失败，前后两项为`Incomplete or invalid Content-Length`，分块项为`Incomplete chunked response`；TLS为true、HTTP200、正文空。命令`go test -run 'TestHTTPSRead/head' -count=1 -v ./third-party/apks/Overt/tests`退出1、3.967秒。

修正：Baidu证书核对请求改为HEAD；响应头完整后结束读取，不按GET表示的Content-Length或Transfer-Encoding要求HEAD正文。真实定位继续GET，原TLS、证书指纹、超时及64KiB限制保留。

回归：相同三项全部通过，读取分别1／1／2次、正文空、HTTP200、TLS为true、错误空。原十项GET全部通过，截断、读取失败、超时和超限仍被拒绝；定位22项及映射查询三项检查通过。同批命令`go test -run 'TestHTTPSRead|TestSSLReply|TestMapLife' -count=1 -v ./third-party/apks/Overt/tests`退出0、12.749秒。

新源码样本的APK构建、真实Baidu响应及无Error页面双轮未完成，整应用验收未通过。

## 2026-10-04分离响应头与正文

输入：Overt2.0.0／940实际APK，普通Pixel6-15 Guest。腾讯定位接口HTTP200、TLS及当前证书通过；实际首次读取408字节，响应头404字节、分隔符4字节、正文0字节，因`Connection: close`直接标记完整，定位JSON解析失败。

步骤：登记Go1.26.8编排NDKr29／Clang21的Android测试，调用实际`zHttps::performRequest()`及非标准容器。TCP、TLS连接与证书校验实际执行，使用链接包装只控制`mbedtls_ssl_read()`返回的记录分段；该测试不作为实际接口响应内容验收。

红灯：十项均失败，每项只读取响应头一次、正文空、无错误，退出1，8.004秒。样本覆盖分离正文、七次读取、末行长度头、EOF定界、分块正文、截断、读取失败、超时、超限及正文NUL。

修正：不以`Connection: close`或读取两次判定正文完整；按长度、分块终止及EOF继续读取，保留时间及64KiB限制，按返回字节数追加正文。读取失败、未完成TLS读取、超限、长度不符和不完整分块均返回错误。

回归：十项全部通过，分离正文实际读取三次、七次读取样本取得完整正文，末行长度及EOF均读取四次；截断、读取失败、超时及超限全部拒绝，NUL字节保持。与定位22项、映射查询三项检查同批退出0，10.662秒。

修正APK实际联网与页面双轮尚未执行，整应用验收未通过。

测试C++入口缩短为`ssl_read_wrap`，保留链接包装所需的ABI符号。重新编译并实际执行相同十项，全部通过，退出0，8.052秒。
