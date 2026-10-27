# PANORAMA typeGPT Prototype 0.4.3 — Windows VST3検証用ソース

**現時点の検証状況**: Linux C++17 DSPビルド・CTest・ASan/UBSan 合格。0.4.2と同じ条件のPCM出力は完全一致。**Windows MSVC/JUCEビルド、VST3生成、pluginval、Studio One実機は未実施**。本ZIPはWindows版の完成バイナリではありません。

今回の変更は、Windows x64でビルド・検証するためのCI整備、JUCE Processor/Editorのスモーク試験の追加、GUI上のホストプリセット変更の反映、バージョン更新です。DSPソースと全32プリセットは変更していません。

- `ci/Build-WindowsVST3.ps1`: Windowsでの一括ビルド・CTest・任意のpluginval検証
- `ci/Validate-WindowsVST3.ps1`: 実VST3バンドルとx64 DLLを確認し、pluginval（GUI込みstrictness 5）を任意実行
- `tests/test_juce_smoke.cpp`: MIDI演奏、32プリセット、状態保存復元、GUI生成・破棄
- `.github/workflows/windows-vst3.yml`: 固定版JUCE 8.0.16-securityとpluginvalでWindows CIを実施（**未実行**）
- `docs/BUILD_WINDOWS_JA.md`: Windows PowerShellビルド手順
- `docs/STUDIO_ONE_ACCEPTANCE_JA.md`: Studio One手動受け入れ検証項目
- `docs/TEST_RESULTS_V043_JA.md`: 実施済みと未実施の切り分け

**保存方針:** 0.4.2以前のZIPを上書きせず、別版で保管します。Windowsインストール時は旧版をバックアップした後、同じプラグインIDを持つ2つのバージョンを同時には配置しないでください。

---

# PANORAMA typeGPT — Prototype 0.4.2

**録音サンプルに依存しない、独立C++ DSPを備えたモジュラー型ソフトシンセサイザー。**

## v0.4.2 品質改善（2026-10-27）

- `Synth::prepare()` でサンプルレート変更後のグラフ構成とフィルター係数を再計算する。
- 48kHzで作成したパッチを再設定せずに96kHzへ再初期化したときも、96kHzで新規初期化した同一パッチとサンプル一致することをテスト。
- 初期化の問題以外の音色生成アルゴリズムは変更しない。
- Windows VST3/Studio Oneの動作は未確認。

## v0.4.1 保守修正（2026-10-26）

オリジナルv0.4.0のファイルを保持し、DSPの音色生成アルゴリズムは変更せず、安全性の修正を加えています。

- JUCEの固定Outputノードに存在しない `gType8` パラメーターを読んでいた問題を修正（ヌル参照リスク）。
- プラグイン音声コールバックから呼ぶ `getPatchFromParameters()` での `std::string` の一時生成を撤廃し、固定長のパラメーターID配列を使用。
- 深度0の音声変調経路について、循環検出と実行順序計算で「接続なし」と同様に扱うように変更（編集中の予期しない無音を防止）。
- 上記の循環接続修正に対してDSP自動テストを追加。
- Linux x64 C++17でビルド、CTest 1/1合格。AddressSanitizer + UndefinedBehaviorSanitizerでも合格。
- v0.4.0 と v0.4.1 の32プリセット、44.1/48/96 kHz、各4096ステレオサンプルのPCMフィンガープリントが一致。

**未確認**：JUCE本体を使用したコンパイル、Windows VST3、Studio One、実際のDAW内メモリ割当測定、エイリアシング等の音質試験。`v0.4.1` は完成版ではありません。

プラグイン識別子（`BUNDLE_ID`, `PLUGIN_CODE`）は0.4から変更していません。ホスト側の互換性は設計意図であり、実機検証はまだ行っていません。

## v0.4 の設計方針

0.1〜0.3の音色を完全保存することは優先しません。機能と音響実験の発展を優先します。Midnight Geometryは、接続構造を参考にしながら基礎発振器を置換した「Midnight Geometry Reforged」を用意しました。

### 追加機能

- **Harmonic Additive Oscillator**：最大12倍音、ナイキスト制限を考慮した部分音の加算合成。倍音の重みを動的に変化できます。
- **Phase Warp Oscillator**：連続的な位相の曲げ方を変更する独自の位相変形発振器。極端なパラメーターではエイリアシングが残り得ます。
- **Dual Modulation Lanes**：LFO A、LFO B、音量エンベロープ、ベロシティ、モジュレーションホイール、ノードのオーディオ信号から2つの変調経路を構築できます。
- **Modulation Destinations**：各ノードのColor、Level、Pitch、Phase。Pitch/Phaseは主に発振器ノード用。非発振器へのPitch/Phase設定は現段階では音色を変えません。
- **DAG Validation**：音声配線と音声変調配線を併せた循環接続を拒否し、トポロジカル順序で実行します。
- **JUCE GUI Source**：2つの変調レーンと新発振器選択をドロップダウンで編集。マウスのドラッグケーブルには未対応。
- **Factory Presets 32**：旧24種類を収録し、新8種類を追加。ただし新しい内部処理に伴い、旧プリセットの音声完全一致は保証しません。

## v0.4 新8音色

25. Midnight Geometry Reforged：Midnight Geometryの音源部をHarmonic Additive + Phase Warpに改造したもの
26. Sine Cathedral：加算倍音、位相変形、二重共鳴
27. Prisma Engine：位相変形、加算、Ring、Fold
28. Polar Machinery：加算、FM、位相変形、共鳴
29. Glass Harmonics：ノイズ励振、加算、共鳴
30. Spectral Migration：加算、位相変形、FM、LFO
31. Phase Orchid：位相変形、加算、デジタル、共鳴
32. Iron Nebula：加算、リング変調、位相変形、金属的共鳴

すべてC++ DSPからPCMを生成。サンプルファイルの読み出しは行っていません。

## 作成・検証環境

- Linux x64、C++17、CMake、Releaseモード、CTest
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build --parallel`
- `ctest --test-dir build --output-on-failure`
- `./build/panorama_render demos 25` など（番号なしの場合は32音色を生成）
- Windows x64 VST3はJUCEを別途取得してビルドしてください。手順は `docs/BUILD_WINDOWS_JA.md`。

**重要：JUCEを組み込んだWindows VST3のビルド、pluginval、Studio One実機テストは未実施です。** ソースは初号機としてのプロトタイプで、DAW上の不具合が残る可能性があります。

## 非実装事項・研究上の制限

- 任意の数のノード、フィードバック用Delay、ケーブルドラッグGUI、完全なSound Space、Inverse Synthesis、Granular、AI音色探索
- 高度なバンド制限Wave Warp、フル機能のMPE、周波数／位相のすべてのノードへの統一適用
- オシレーターのあらゆる音高・変調条件でのエイリアシング排除
- 音質の客観的優劣や商用完成度の保証

この段階で「万能な最高傑作」の完成を宣言せず、ユーザーの試聴とWindows VST3試験を経て改良を進めます。
