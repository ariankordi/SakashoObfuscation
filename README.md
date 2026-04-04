# SakashoObfuscation

### What is this
Decodes and encodes some stupid proprietary obfuscation that DeNA put on some HTTP APIs for their games as a part of their "Sakasho" library.

Made primarily for Miitomo, but it may help for other DeNA games such as _Duel Masters PLAY'S (デュエプレ)_. I don't know.

### Why is this
Have you ever been a 12-year-old Arian Kordi intercepting Miitomo traffic with mitmproxy in 2016/2017/2018 in the hopes of making a revival when it shuts down, just to find out that all of the responses look like a garbled jumbled mess? And the only people who have ever figured this out have not shared any details, or source code, or have been able to reliably host their server in the past ever?

<!--
    Older versions:
    * 2024-09-14: https://gist.github.com/ariankordi/0b990239daa1f69d571c7de3bec57cc4
    * 2025-12-28: https://gist.github.com/ariankordi/b9b21343bef4f0908607f4d6c9b047c0
-->

Happy 10th birthday, Miitomo! You deserve someone who can do you better. This is better. This API obfuscation is the building block for a new revival.

Well... whoever's interested in that will also have to handle NPF (login service), and the API URLs themselves are further obfuscated somewhere in libsakasho.so for Android and in the binary for the iOS version. So this isn't actually all you will need, but it's still one of the most important parts.

Because this decoder/encoder is written in the [Fusion Programming Language](https://github.com/fusionlanguage/fut), it should Just Work(TM) in whatever language you choose to use for your server, or you can use a hacky solution to further transpile it like I did in SakashoObfuscation.go.

### How do I use
If you're asking, then this isn't for you. Sorry.
