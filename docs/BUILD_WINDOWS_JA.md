# PANORAMA typeGPT — Prototype 0.4.3 Windows x64 VST3 検証手順

## 検証状態の区別

この配布ソースを作成したLinux環境には、Windows/MSVC、Windows用VST3実行環境、JUCEソース、Studio Oneがないため、**本配布時点でWindowsバイナリはビルドも検証もされていません。** GitHub Actionsの設定やスモークテストコードがあることは実行成功を意味しません。前版0.4.2のDSPコードとプリセットは維持しています。

## Windows側の準備

1. Windows 10/11 x64、Visual Studio 2022＋「C++によるデスクトップ開発」、CMake、Git。
2. JUCE 8.0.16-security のソース。例えばGitで取得：

```powershell
git clone --depth 1 --branch 8.0.16-security https://github.com/juce-framework/JUCE.git C:\SDK\JUCE
```

3. オプション：pluginval v1.0.4のWindows配布物を展開し、`pluginval.exe` のフルパスを把握する。

公式参照：
- JUCE: https://github.com/juce-framework/JUCE/releases/tag/8.0.16-security
- pluginval: https://github.com/Tracktion/pluginval/releases/tag/v1.0.4

JUCEのライセンスを確認して使用してください。JUCE/SDKソースは本ZIPに再配布していません。

## 推奨：一括実行（Windows PowerShell）

`PANORAMA_typeGPT_v0.4.3` のルートで以下を実行します。パスは実環境に合わせて変更します。

```powershell
powershell -ExecutionPolicy Bypass -File .\ci\Build-WindowsVST3.ps1 `
  -JucePath "C:\SDK\JUCE" `
  -PluginvalExe "C:\SDK\pluginval\pluginval.exe"
```

このスクリプトは構成→MSVC Releaseビルド→CTest（DSPおよびJUCE Processor/Editorスモーク）→VST3バンドル構造検査→pluginval strictness 5（GUIテスト有効）の順に実行し、全検証合格時のみZIPを生成します。

pluginvalがない場合は `-PluginvalExe` を省略できますが、**pluginval未検証**のままです。この場合でもバイナリを生成できますが、完成・合格の証明ではありません。

GitHub Actionsでの自動ビルド：`.github/workflows/windows-vst3.yml`。リポジトリーの `main` にこのZIP内容を配置し、`Actions → PANORAMA typeGPT 0.4.3 Windows VST3 and JUCE Validation → Run workflow` から実行します。通常DSPテストとJUCEスモークテストを先に行い、VST3の実在検査、pluginvalを実行します。成功時のWindows x64 VST3は **ActionsのArtifacts** から取得できます。失敗した場合は失敗ログだけを保存します。

## 手動ビルド・テスト

```powershell
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 `
  -DPANORAMA_BUILD_VST3=ON -DPANORAMA_BUILD_JUCE_SMOKE=ON `
  -DPANORAMA_JUCE_PATH="C:/SDK/JUCE"
cmake --build build-win --config Release --parallel 4
ctest --test-dir build-win -C Release --output-on-failure --timeout 180
.\ci\Validate-WindowsVST3.ps1 -BuildRoot build-win `
  -PluginvalExe "C:\SDK\pluginval\pluginval.exe"
```

VST3の出力予想先：`build-win/PANORAMA_typeGPT_Plugin_artefacts/Release/VST3/PANORAMA typeGPT v0.4.3.vst3`。

Windows側で検証すべき点：実ファイルが生成されること、pluginval GUIあり strictness 5 PASS、44.1/48/96kHz・64/128/256/512/1024 sampleのMIDI演奏とパラメータ変更、プリセットの保存復元、GUI開閉、CPU安定性。**pluginval PASS と Studio Oneの実機PASSは別物です。**

## Studio Oneへの導入

1. アーカイブから `.vst3` バンドルを展開。
2. Windows標準のVST3フォルダー `C:\Program Files\Common Files\VST3\` への導入前に旧版を別の場所へバックアップし、同一プラグインIDの複数版を同時に置かないようにします。
3. Studio Oneを起動し、プラグインを再スキャン。必要ならブロックリストを確認。
4. `docs/STUDIO_ONE_ACCEPTANCE_JA.md` の実機テスト項目を一つずつ確認。

確認に失敗した場合は、MSVCビルドログ、CTestログ、pluginvalログ、Studio Oneのエラーメッセージを保存し、バイナリ完成扱いにはしないでください。
