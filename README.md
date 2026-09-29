# NKB Twin — Twin Reverb系クリーンアンプ

Twin Reverbのブラックフェイス系クリーンを参考にした、ギター用アンプシミュレーターです。広いクリーンヘッドルーム、Brightのボリューム依存、高域の抜け、ミッドを少し抑えたトーンと2x12スピーカーの簡易ボイシングを狙っています。メイン画面はNormal入力、Bright、Volume／Treble／Middle／Bass、黒い操作パネル、銀色グリル、赤いパイロットランプを持つTwin Reverbの外観を参考にしています（[Fender製品仕様](https://www.fender.com/products/65-twin-reverb)）。リバーブとトレモロは搭載していません。特定製品の回路や音を完全再現するものではありません。

ギターのモノラル入力をアンプ処理し、同じ音を左右の出力へ送ります。オーディオデバイスがステレオ入力ペアを公開している場合は、入力1・2のうち信号レベルが大きい側を自動で選びます。

信号の順番は **エフェクター → アンプ → キャビネット** です。連続値コントロールの初期値はすべて **5.0**、エフェクターとカスタムIRはオフで起動します。画面右上の **RESET** で初期状態へ戻せます。

## 操作子と初期値

| 操作子 | 初期値 | 内容 |
| --- | ---: | --- |
| Volume | 5.0 | プリアンプの押し込みと音量 |
| Treble | 5.0 | 高域 |
| Middle | 5.0 | 中域 |
| Bass | 5.0 | 低域 |
| Bright | オフ | 高域を少し持ち上げる |
| Blues Driver: Level / Gain / Tone | 各5.0 | エフェクターページのBD-2系モデル。初期状態はオフ |
| OverDrive: Level / Drive / Tone | 各5.0 | エフェクターページのOD-3系モデル。初期状態はオフ |
| Custom IR | オフ | キャビネットページから読み込むキャビネットIR |

エフェクターページではBD-2系の青いペダルの隣にOD-3系の黄色いペダルを並べています。それぞれ独立したフットスイッチとLevel／Gain（OD-3はDrive）／Toneを持ち、両方オンにした場合は **BD-2 → OD-3 → アンプ** の順に処理します。BD-2は公開回路図と[回路解析記事](https://note.com/paul_white_stone/n/ne03f047e33a8)を参考にした段構成です。OD-3はメーカーが説明する[デュアルステージ構成](https://www.boss.info/global/products/od-3/)と、添付されたペダル外観・基板レイアウトを参考に、二段のソフトクリップ、Tone調整、Levelを組み合わせています。どちらも部品単位の回路シミュレーションではなく、回路の特徴を狙ったDSP近似です。

エフェクターページは実機ペダルに近い縦長サイズで、上段にLevel／Gain、中央下段にToneを配置しています。信号のページ順は **エフェクター → アンプ → キャビ** です。

キャビネットページの **LOAD IR** からモノラルまたはステレオのWAV/AIFFファイルを読み込めます。読み込み後はCustom IRが有効になり、内蔵Twin 2x12スピーカー特性と切り替わります。**CLEAR** で内蔵特性へ戻します。IRのファイルパスとコントロール設定はスタンドアロン版の状態に保存されます。

入力と出力のレベル差が大きい場合は、オーディオデバイス側で入力ゲインを調整してください。各ノブとスイッチは状態保存に対応します。

## VST3版（Cubase）

GitHub Releasesから `NKB-Twin-VST3-v1.1.0-Windows-x64.zip` をダウンロードして展開し、中の `NKB Twin.vst3` フォルダー全体をVST3プラグインの検索場所へコピーしてください。標準のWindows共有フォルダーは `C:\Program Files\Common Files\VST3` です。Cubaseを再スキャンして、オーディオトラックのインサートエフェクト **NKB Twin** として読み込みます。ギターを接続した入力を選び、出力をステレオに設定してください。

## スタンドアロン版

ASIOに対応しています。Standalone版を起動し、ウィンドウ上部の **Options → Audio/MIDI Settings...** で Audio device type を **ASIO** に切り替え、利用するASIOドライバーと入出力チャンネルを選んでください。このPCでは **Komplete Audio 6**、**Realtek ASIO**、**Steinberg built-in ASIO Driver**、**Generic Low Latency ASIO Driver** が列挙されました。ドライバーが一覧にない場合は、そのオーディオインターフェース／機器のASIOドライバーをWindowsにインストールしてください。

**TEST TONE** をオンにすると440 Hzのテスト信号が左右に出ます。画面の **OUT L** と **OUT R** が両方動けば、アプリから左右の出力まで信号が届いています。片側しか動かない場合はデバイス側の出力チャンネル設定を確認してください。両方動くのに片側からしか聞こえない場合は、ASIOの出力割り当て、接続先、ケーブル／ヘッドフォンを確認してください。テスト信号をオフにしてからギターを入力に接続し、入力チャンネルを有効にして演奏します。入力メーターが動かない場合は入力デバイスとチャンネル設定を確認してください。

## Windowsでビルド

必要なもの：CMake 3.22以降、Git、Visual Studioの **C++によるデスクトップ開発** ワークロード、Windows SDK。Visual Studio Developer PowerShellまたはDeveloper Command Promptでこのフォルダーから実行します。初回のCMake設定でJUCE 9.0.2を取得します。

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target NkbTwin_Standalone NkbTwin_VST3
```

スタンドアロンアプリは `build/NkbTwin_artefacts/Release/Standalone/`、VST3プラグインは `build/NkbTwin_artefacts/Release/VST3/NKB Twin.vst3` に出力されます。Cubaseで使うには `.vst3` フォルダーをVST3プラグインの検索場所へ置き、プラグインスキャンを実行してください。

JUCEはビルド時に公式リポジトリから取得します。JUCEのライセンス条件を確認して利用してください。

ASIO対応部分にはSteinberg ASIO SDK由来のコードが含まれるため、ASIO版を配布する場合はSteinberg/JUCEのライセンス条件を確認してください。
