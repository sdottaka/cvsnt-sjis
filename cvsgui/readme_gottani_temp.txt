Mon Feb 09 2004, hitoh.

以下、Gottani_config.h に入れようと思ったがあまりそこに入れても意味がないことに
気付いた。ので、このように説明だけ切り出してきた。プロジェクトファイルで指定
して下さい。

================================================================================
/// @brief * CreateProcess の lpCommandLine 問題。
///
/// どうやら cvsgui_protocol 化されたがために、これまでとは別種の問題を v1.3 は持つことになって
/// しまったようだ。多分 NT 系固有と思われるが、lpCommandLine を CreateProcess に渡す際に、
/// EUC 文字列を受け取って OS が勝手に混乱してしまうらしい。(NT系だけだろうと言う根拠は、
/// NT 系ではそこに UNICODE と ANSI への変換が起こるという点。)
///
/// これは cvsgui_protocol 内の cvs_process_run でのコマンドライン文字列と、CVS クライアントに
/// 行ってからのコマンドライン文字列を比較するとわかる。cvs_process_run で CreateProcess する
/// 前までは間違いなく EUC なのに、cvsgui.c へ渡ってみると、既に argv[i] が壊れている。
///
/// で、どうやって FIX するか。相手が UNICODE だとすると、もはや 8 bit コード自体危ないと判断
/// したので、仕方がないので、URI エンコードを仮に使うことにした。明らかに牛刀であり、なおかつ、
/// 文字列が最大3倍に膨れ上がってしまうので、明らかに理想とは程遠いが、当面これで逃げることに
/// する。
#define WINCVS_GOTTANI_TEMPORARILY_FIX_COMMANDLINE_PROBLEM /**/
================================================================================

なお、問題意識が、↑を書いたときはかなり抜けていた。そもそも、どうやって
URI をやり取りし合っているのかを知るのかと、もし URI なんか知らんという CVS
クライアントだった場合にどうするか、の問題の方が、遥かに大きい。

このように考えることにした。
・URI エンコードを渡している、ということを WinCvs 側が余分なオプションを渡すことで
  CVS クライアントに知らしめる。
  → こうすれば、そのオプションを知らない CVS クライアント + cvsgui は単にエラーを
     返すだけであって、何も知らずに URI エンコードされたままサーバに送信してしまう
     危険はなくなる。
・そして、URI エンコードするかどうかを、WinCvs 側の UI から設定する。

その余分な引数をどこで扱うか、は微妙なのだが、CVSクライアント側そのものか cvsgui_protocol
か迷って、結局後者にした。最終的に CVSクライアント+cvsgui_protocol というトータルとして
このオプションを受け付けられればいい、ということに尽きるのではあるが、例えば CVSNT
をメンテナンスする際に、コマンドライン問題に限れば、この特殊な cvsgui_protocol を使うだけ
でどうにか『弱った文字列を持ったコマンドライン』を扱えるようになるのは、まぁ嬉しいこと
かもしれないな、と思う。

# それだったら --noknjwrp とかもそうだろ、って話もあったりするんだけど、それは歴史的な
# 事情、ってやつでして。
