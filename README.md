このプロジェクトは、Hermes Agentとcodexの組み合わせがスクラッチで自動作成したものです。

# ebikin ES8311 ESP-IDF コンポーネント

ES8311 の **レジスタ/I²C 制御だけ**を扱う再利用可能な ESP-IDF コンポーネントです。

`components/ebikin_es8311/` をそのまま他のプロジェクトのローカルまたは Git コンポーネント依存に指定できます。

ライセンスは、GPL-3.0 です。

I2S チャンネル/クロックの作成・停止、録音、DMA、FIFO、Opus、アップロード、起動時ウォームアップ処理は **含みません**。

ESP-IDF 5.4.3 / `esp-idf-lib/i2cdev ==2.1.2` の新 I²C master API でコンパイル確認済みです。

依存関係は Component Manager が自動で解決します。

## コンポーネントの導入

`ebikin_es8311` は、ESP-IDF Component Manager を使用して、任意の ESP-IDF プロジェクトから利用できます。

リポジトリ：

https://github.com/kinneko/ebikin-es8311-idf

### 1. Git リポジトリからの導入（推奨）

利用する ESP-IDF プロジェクトの `main/idf_component.yml` に、以下の依存関係を追加します。

```yaml
dependencies:
  ebikin_es8311:
    git: https://github.com/kinneko/ebikin-es8311-idf.git
    path: components/ebikin_es8311
    version: v0.1.0-pre1
```

`version` には Git タグ、ブランチ名、またはコミットハッシュを指定できます。安定したビルドを再現するため、特定のタグまたはコミットハッシュへの固定を推奨します。

依存コンポーネントは ESP-IDF Component Manager により自動的に取得されます。

### 2. ローカルディレクトリからの導入

コンポーネントをローカルで開発・変更する場合は、リポジトリをクローンします。

```bash
git clone https://github.com/kinneko/ebikin-es8311-idf.git
```

利用するプロジェクトの `main/idf_component.yml` に、コンポーネントへの相対パスまたは絶対パスを指定します。

```yaml
dependencies:
  ebikin_es8311:
    path: /path/to/ebikin-es8311-idf/components/ebikin_es8311
```

`path` は `idf_component.yml` が置かれているディレクトリを基準に指定します。実際の配置に合わせて変更してください。

### 3. CMake での依存関係

利用するコンポーネントの `CMakeLists.txt` に `ebikin_es8311` を追加します。

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES ebikin_es8311
)
```

`main` 以外のコンポーネントから利用する場合も同様です。公開ヘッダーで API を使用しない場合は `PRIV_REQUIRES` も使用できます。

ソースコードからは次のようにインクルードします。

```c
#include "ebikin_es8311.h"
```

### 4. I²C と i2cdev の依存関係

本コンポーネントは `esp-idf-lib/i2cdev` を使用します。現在の `idf_component.yml` ではバージョン `2.1.2` に固定されています。

親プロジェクトや他のコンポーネントでも `i2cdev` を利用する場合は、依存バージョンの整合性を確認してください。

同じ I²C バスには、別々のデバイス descriptor を使用して複数のデバイスを接続できます。例えば、ES8311 と RTC（PCF8563、アドレス `0x51`）は、適切なアドレスとバス設定を行うことで同一の I²C バスを共有できます。

I²C バスを共有する場合は、以下に注意してください。

- 同じ I²C ポートに、競合する別の I²C ドライバを導入しないでください。
- 本コンポーネントは `i2cdev_init()` を呼び出しますが、`i2cdev_done()` は呼び出しません。
- `ebikin_es8311_deinit()` は自身の descriptor のみを解放し、他のデバイスが使用する I²C バス全体は停止しません。
- 複数タスクから I²C デバイスを操作する場合、タスク間の同期および終了処理は呼び出し元アプリケーションの責任です。

I²C ポート番号、SDA/SCL の GPIO 番号、クロック周波数、デバイスアドレスは、利用するハードウェア構成に合わせて設定してください。

### 5. ビルド

ESP-IDF の環境を初期化した後、プロジェクトのルートディレクトリでビルドします。

```bash
idf.py reconfigure
idf.py build
```

Component Manager が依存関係を解決し、`ebikin_es8311` と必要な依存コンポーネントをビルドに組み込みます。

## API と使用方法

`ebikin_es8311` は、ES8311 オーディオコーデックの初期化、レジスタ設定、ミュート、音量調整を行う ESP-IDF コンポーネントです。

公開ヘッダー：

`components/ebikin_es8311/include/ebikin_es8311.h`

各 API は `esp_err_t` を返します。引数が不正な場合は `ESP_ERR_INVALID_ARG`、I²C 通信に失敗した場合は下位ドライバのエラーを返します。

### 1. API 一覧

| API | 機能 |
| --- | --- |
| `ebikin_es8311_init()` | I²C デバイスの初期化とハンドル生成 |
| `ebikin_es8311_probe()` | ES8311 のデバイス ID 確認 |
| `ebikin_es8311_read_register()` | レジスタの読み取り |
| `ebikin_es8311_configure()` | コーデックのレジスタ設定 |
| `ebikin_es8311_set_dac_mute()` | DAC のミュート制御 |
| `ebikin_es8311_set_dac_volume()` | DAC の音量設定 |
| `ebikin_es8311_stop_mute()` | 再生停止時のミュート処理 |
| `ebikin_es8311_deinit()` | ハンドルと関連リソースの解放 |

### 2. 初期化から終了までの呼び出し順

基本的な処理順序は次のとおりです。

1. **初期化** — `ebikin_es8311_init()` で I²C descriptor とハンドルを生成する。
2. **デバイス確認** — `ebikin_es8311_probe()` で ES8311 が接続されていることを確認する。
3. **I²S 開始** — アプリケーション側で I²S を初期化し、BCLK/WS の供給を開始する。
4. **コーデック設定** — `ebikin_es8311_configure()` で ES8311 のレジスタを設定する。
5. **再生準備・開始** — 必要に応じて音量を設定し、ミュートを解除する。
6. **再生停止** — `ebikin_es8311_stop_mute()` でミュートした後、I²S を停止する。
7. **終了処理** — コーデックを使用しなくなった時点で `ebikin_es8311_deinit()` を呼び出す。

**重要：** `ebikin_es8311_configure()` を呼び出す前に、I²S の BCLK/WS を有効にしてください。本コンポーネントは I²S ドライバの初期化・開始・停止を行いません。

### 3. 使用例

以下は ESP-IDF アプリケーションから利用する例です。

```c
#include "esp_err.h"
#include "driver/i2c.h"
#include "ebikin_es8311.h"

static ebikin_es8311_t *codec = NULL;

static const ebikin_es8311_config_t codec_config = {
    .port = I2C_NUM_0,
    .sda_gpio = 38,
    .scl_gpio = 39,
    .address = 0x18,
    .bus_hz = 100000,
    .internal_pullups = true,
};

// 1. 初期化
ESP_ERROR_CHECK(ebikin_es8311_init(&codec_config, &codec));

// 2. デバイス確認
ebikin_es8311_id_t id = {0};
ESP_ERROR_CHECK(ebikin_es8311_probe(codec, &id));

// 3. アプリケーション側で I2S を初期化・有効化
//    BCLK/WS が供給されていることを確認する。

// 4. コーデック設定
ESP_ERROR_CHECK(ebikin_es8311_configure(codec));

// 5. 音量設定とミュート解除
ESP_ERROR_CHECK(ebikin_es8311_set_dac_volume(codec, 128));
ESP_ERROR_CHECK(ebikin_es8311_set_dac_mute(codec, false));

//    アプリケーション側で音声の再生・録音を行う。

// 6. 再生停止時は、I2S を停止する前にミュート
ESP_ERROR_CHECK(ebikin_es8311_stop_mute(codec));

//    アプリケーション側で I2S を停止する。

// 7. アプリケーション終了時に解放
ESP_ERROR_CHECK(ebikin_es8311_deinit(&codec));
```

この例の GPIO、I²C ポート、アドレス、音量値は設定例です。実際の接続に合わせて変更してください。

I²S の初期化、音声データ転送、再生開始・停止処理は、アプリケーション側で実装する必要があります。

### 4. 各 API の詳細

#### 初期化：`ebikin_es8311_init()`

指定された I²C ポート、GPIO、7-bit アドレス、通信速度、内部プルアップ設定を使用してデバイス descriptor を確保します。

- 出力ハンドルは事前に `NULL` に初期化してください。
- この関数では ES8311 のハードウェアレジスタを書き換えません。
- 通常、アプリケーション起動時に一度だけ呼び出します。

#### デバイス確認：`ebikin_es8311_probe()`

ES8311 の識別レジスタ `0xFD`、`0xFE`、`0xFF` を順番に読み取ります。

- デバイス ID `0x83` / `0x11` を検証します。
- ID が一致しない場合は `ESP_ERR_NOT_FOUND` を返します。
- レジスタの読み取りに成功した場合は、ID が一致しなくても読み取った値を出力します。

#### レジスタ読み取り：`ebikin_es8311_read_register()`

指定されたレジスタから 1 バイトを読み取ります。

デバイスの動作確認や、設定値を調べる際に利用できます。

#### コーデック設定：`ebikin_es8311_configure()`

ES8311 の初期設定を行います。

現在の実装は、次の I²S 条件を前提としています。

| 項目 | 設定 |
| --- | --- |
| サンプリング周波数 | 16 kHz |
| データ形式 | 32-bit × 2 スロット |
| BCLK | 1.024 MHz |
| ES8311 の動作モード | Slave |

初期化処理では、まずレジスタ `0x00` に `0x1F` を書き込み、20 ms 待機してから、残りのレジスタを所定の順番で設定します。

注意事項：

- 呼び出し前に I²S の BCLK/WS を供給してください。
- `probe()` や I²S のウォームアップは実行しません。
- 設定途中でエラーが発生した場合は直ちに処理を中断します。ロールバックは行いません。
- 設定完了時はレジスタ `0x31 = 0x60` のミュート状態を維持します。

現在の設定シーケンスは上記の音声フォーマット用に固定されています。異なるサンプリング周波数や I²S フォーマットで利用する場合は、設定内容の変更が必要です。

#### ミュート制御：`ebikin_es8311_set_dac_mute()`

DAC のミュート状態を変更します。

| 引数 `muted` | レジスタ `0x31` | 動作 |
| --- | --- | --- |
| `true` | `0x60` | ミュート |
| `false` | `0x00` | ミュート解除 |

ミュートを解除する前に、I²S の動作状態や音量設定を確認してください。

#### DAC 音量：`ebikin_es8311_set_dac_volume()`

DAC 音量レジスタ `0x32` に指定値を書き込みます。

- 指定範囲：`0` ～ `255`
- レジスタ値を直接指定する API です。
- `ebikin_es8311_configure()` からは呼び出されません。
- 音量を変更してもミュートは解除されません。

#### 再生停止：`ebikin_es8311_stop_mute()`

再生停止前にレジスタ `0x31` に `0x60` を書き込み、DAC をミュートします。

I²S の停止は行わないため、アプリケーション側でミュート処理の後に I²S を停止してください。

#### 終了処理：`ebikin_es8311_deinit()`

コンポーネントが所有する I²C デバイス descriptor と mutex を解放します。

- 正常終了時はハンドルを `NULL` に設定します。
- NULL ハンドルが指定された場合も成功します。
- リソースの削除に失敗した場合はハンドルを保持し、再試行できる状態にします。
- 共有している I²C バス全体は停止しません。
- I²S ドライバの状態は変更しません。

通常の再生停止では呼び出さず、コンポーネントを完全に終了する際に使用してください。

### 5. I²C バス共有とスレッド安全性

本コンポーネントは `i2cdev` を使用します。同じ I²C ポートに接続された RTC などのデバイスとは、それぞれ独立した descriptor を使用することでバスを共有できます。

ただし、次の制限があります。

- 同じ codec ハンドルに対する複数タスクからの並行操作はサポートしません。
- `deinit()` 実行中に他の API を呼び出さないでください。
- コーデック使用タスクの停止・同期はアプリケーション側で管理してください。
- `deinit()` は共有 I²C バスを停止しないため、他のデバイスは引き続き利用できます。

### 6. I²S ウォームアップについて

I²S 開始直後に一定量の受信データを破棄するウォームアップ処理は、アプリケーション側で実装します。

例えば、16 kHz で 100 ms のウォームアップを行う場合は、1,600 サンプルに相当します。

この処理は `ebikin_es8311_configure()` に含まれません。

再生・録音のタイミング管理、受信バッファの処理、サンプル位置の補正もアプリケーション側で行ってください。


## ライセンスと出典

### ライセンス

本リポジトリの C 実装は、ebikin によるオリジナル実装であり、**GPL-3.0-only** で公開しています。

元のアプリケーションから ES8311 の制御処理を独立したコンポーネントとして分離したものです。アプリケーション固有の音声処理や I²S 制御コードは含まれていません。

### 参考資料

ES8311 の設定値や初期化手順の調査にあたり、以下の公開ソースコードを参考にしました。

**M5Stack M5Atomic-EchoBase v1.0.2**

- リポジトリ：https://github.com/m5stack/M5Atomic-EchoBase
- 参照コミット：`bf4b610495c5499ecece46a23f6fd446de19284a`
- `src/M5EchoBase.h` / `src/M5EchoBase.cpp`：ピン設定、クロックの初期化順序
- `src/es8311.cpp`：ES8311 の設定値、DAC レジスタ

M5Stack のライブラリ本体は MIT ライセンス、参照先に含まれる Espressif の ES8311 コードは Apache-2.0 ライセンスです。

これらのソースファイルは本リポジトリにコピー・同梱しておらず、M5Stack の Arduino ライブラリにも依存していません。

各参照元の著作権およびライセンスは、それぞれの権利者に帰属します。

### 外部依存コンポーネント

本コンポーネントは、I²C 通信に以下のライブラリを利用します。

| コンポーネント | バージョン | ライセンス |
| --- | --- | --- |
| [esp-idf-lib/i2cdev](https://github.com/esp-idf-lib/i2cdev) | 2.1.2 | MIT |
| ESP-IDF | 対応する SDK バージョン | 各構成要素のライセンスに従う |

`i2cdev` は ESP-IDF Component Manager により依存コンポーネントとして取得されます。

依存ライブラリや ESP-IDF 自体のソースコードは、本リポジトリでは変更・同梱していません。

Component Manager が `managed_components/` に取得するファイルも、Git の管理対象には含めていません。
