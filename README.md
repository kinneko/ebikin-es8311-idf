# ebikin ES8311 ESP-IDF コンポーネント

ES8311 の **レジスタ/I²C 制御だけ**を扱う再利用可能な ESP-IDF コンポーネント。`components/ebikin_es8311/` をそのまま他のプロジェクトのローカルまたは Git コンポーネント依存に指定できる。GPL-3.0-only。`LICENSE` は GNU 公式 https://www.gnu.org/licenses/gpl-3.0.txt から取得した全文（取得時 SHA-256: `3972dc9744f6499f0f9b2dbf76696f2ae7ad8af9b23dde66d6af86c9dfb36986`）。

I2S チャンネル/クロックの作成・停止、録音、DMA、FIFO、Opus、アップロード、起動時ウォームアップ処理は **含まない**。ESP-IDF 5.4.3 / `esp-idf-lib/i2cdev ==2.1.2` の新 I²C master API でコンパイル確認済み。依存は Component Manager が取得する。LifeLog 統合後の実機試験結果は後述。

## 取り込み

LifeLog の現行ローカル作業ツリーは次のタグ付きGit依存を使用する（LifeLog自体は未公開）:

```yaml
dependencies:
  ebikin_es8311:
    git: https://github.com/kinneko/ebikin-es8311-idf.git
    path: components/ebikin_es8311
    version: v0.1.0-pre1
```

`firmware/main/CMakeLists.txt` の `REQUIRES` に `ebikin_es8311` を追加する。ローカル開発でGit依存を置き換えたい場合は（LifeLogと本リポジトリを `/Users/kinneko/work/` に並べた例）:

```yaml
dependencies:
  ebikin_es8311:
    path: ../../../ebikin-es8311-idf/components/ebikin_es8311
```

Git 依存を使う親アプリでは、`i2cdev` 依存とのバージョン整合を確認する。本コンポーネントの manifest は 2.1.2 に固定。RTC (PCF8563 0x51) は同じ i2cdev の別 descriptor として I2C0/SDA38/SCL39 を共有できる。別の I²C driver を同じ port に導入しない。コンポーネントは `i2cdev_init()` を呼ぶが、`i2cdev_done()` は呼ばない。`deinit` は自身の descriptor だけを削除する。他 descriptor のバスを停止しない。I²C 利用タスクの停止/同期はアプリ側の責任。

### API と呼出順

ヘッダー: `components/ebikin_es8311/include/ebikin_es8311.h`。全関数は `esp_err_t` を返す。引数不正は `ESP_ERR_INVALID_ARG`、I²C 障害は下位エラーを伝播する。ハンドルは並行操作・deinit 中の操作に対応しない（呼出元で同期）。

| API | 動作 |
| --- | --- |
| `ebikin_es8311_init(&config, &codec)` | 明示 port、SDA/SCL GPIO、7-bit address、bus_hz、internal_pullups で i2cdev descriptor を確保。`codec` は事前に NULL。ハードウェア書込みなし。 |
| `ebikin_es8311_probe(codec, &id)` | 0xfd/0xfe/0xff を順に読み、0x83/0x11 を検証。不一致は `ESP_ERR_NOT_FOUND`。読み取り成功時は不一致でも ID 値を出力。 |
| `ebikin_es8311_read_register(codec, reg, &value)` | 診断用にレジスタを1バイト読み取る。 |
| `ebikin_es8311_configure(codec)` | **呼出元が I2S BCLK/WS を先に有効化**。LifeLog の固定 16 kHz / 32-bit×2 wire slots / BCLK=1.024 MHz の slave 用 0x00=0x1f → 20ms → 残りの設定を元と同順序で書く。失敗時は即停止・ロールバックなし。probe や I2S warmup は含まない。末尾 0x31=0x60 の mute を保持。 |
| `ebikin_es8311_stop_mute(codec)` | 停止時 0x31=0x60。呼出元はその後 I2S を停止。 |
| `ebikin_es8311_set_dac_mute(codec, muted)` | 0x31へmuteなら0x60、解除なら0x00。解除前に再生条件・音量を呼出元が確認する。 |
| `ebikin_es8311_set_dac_volume(codec, raw)` | 任意の DAC register 0x32 に raw 0..255 を書く。`configure` からは呼ばれない。ミュート解除ではない。 |
| `ebikin_es8311_deinit(&codec)` | 自身の device/mutex だけ解放、成功で codec=NULL。NULL ハンドルは成功。削除失敗ではハンドルを保持し再試行可能。I2S は変更しない。 |

LifeLog 側の置換例（アプリ側の既存 I2S 構築・RX/TX handle 維持・warmup 1600 samples は残す）:

```c
static ebikin_es8311_t *codec;
static const ebikin_es8311_config_t codec_config = {
    .port = I2C_NUM_0, .sda_gpio = 38, .scl_gpio = 39,
    .address = 0x18, .bus_hz = 100000, .internal_pullups = true,
};

// アプリ起動または audio owner task 内で一度初期化。停止/再開間は handle を維持。
ESP_ERROR_CHECK(ebikin_es8311_init(&codec_config, &codec));
ebikin_es8311_id_t id = {0};
ESP_ERROR_CHECK(ebikin_es8311_probe(codec, &id));
// アプリが RX/TX I2S を enable し BCLK/WS が出力中になってから:
ESP_ERROR_CHECK(ebikin_es8311_configure(codec));
// アプリ側で既存の RX warmup 100ms (1600 samples) を実施し offset に加算。
// 停止時: mute → I2S disable。RTC はその間も同じ i2cdev bus を利用可能。
ESP_ERROR_CHECK(ebikin_es8311_stop_mute(codec));
// アプリ終了時に全 codec 操作を停止してから（通常の audio stop では呼ばない）:
ESP_ERROR_CHECK(ebikin_es8311_deinit(&codec));
```

## 検証

```sh
python3 -m unittest discover -s tests -v
source /Users/kinneko/work/tab5/esp-idf-5.4.3/export.sh
idf.py -C examples/probe set-target esp32s3 build
```

`examples/probe` は I2S clock を供給しないため **ID を読むだけ**。`configure` は呼ばず、このexample自体は flash していない。Python テストは固定レジスタ列の静的回帰検査で、実 I²C transaction/RTC 同居動作を単独では検証しない。

### LifeLog統合と実機（2026-10-14）

- LifeLogの `firmware/main/idf_component.yml` は公開tag `v0.1.0-pre1`をGit参照し、`firmware/dependencies.lock`でcommit `49f0366e8f2a8e5e0c575c7f512b95b811cf4ce0`（tagの指すcommit）に固定。`audio_capture.c`の直書きES8311レジスタI/Oを公開APIへ移し、I2S/録音/Opus/撮影/RTCはアプリ内に維持。ローカル依存と公開Git依存の双方で`idf.py -C firmware build`成功（LifeLog `patches/lifelog-component-build.log`, `patches/lifelog-git-component-build.log`）。
- アプリ領域だけをUSB書き込みし、esptool hash verified。書込前アプリはLifeLogの`patches/pre-es8311-component-app.bin`にバックアップ（NVS消去なし）。USB実機でES8311 FD=0x83/FE=0x11、GC0308 PID=0x9b、RTC有効、Opus音声3件と画像1件の同時動作を確認。4件のAPI確定・S3原bytes/ハッシュ/メタデータ読戻し成功。終了時、音声/周期撮影/送信/保留は停止、overrun/error=0。証跡`/Users/kinneko/work/ebikin-lifelog/tests/runtime/es8311-component/smoke-20261014/summary.json`。単体exampleにはI2Sを供給していないため、この検証はLifeLog統合版によるもの。
- GPL-3.0-onlyの本コンポーネントを組み込むLifeLogファームウェアを配布する際は、LifeLog側の著作権・ライセンス表記と対応するソース提供条件を整理する必要がある。現時点でLifeLog全体に自動的にLICENSEを付けたり、法的判断を確定したりはしていない。

## 出典とライセンス

- **このリポジトリの C 実装は ebikin original / GPL-3.0-only。** 移植元はローカル `/Users/kinneko/work/ebikin-lifelog/firmware/main/audio_capture.c` の分離前 codec 部分（当時72–125行、特に75、78–87、103–115、124行）。手順の背景は同プロジェクト `docs/stage4.md`。アプリの音声処理や I2S コードは持ち込まない。LifeLog側は現在この独立リポジトリの公開tagに固定して統合済み。
- **設定調査の参照資料（コピー・リンクしていない）:** M5Stack `M5Atomic-EchoBase` v1.0.2、commit `bf4b610495c5499ecece46a23f6fd446de19284a`、https://github.com/m5stack/M5Atomic-EchoBase/tree/bf4b610495c5499ecece46a23f6fd446de19284a 。`src/M5EchoBase.h/.cpp` の pin/clock 開始順、`src/es8311.cpp` の codec 設定・DAC register を参照。M5Stack wrapper/root は MIT、**ES8311 reference source は Apache-2.0**（Espressif）。当リポジトリにそのソースファイルを含めず Arduino ライブラリも依存しない。この明示は M5Stack の著作権・ライセンスを GPL と読み替えるものではない。
- 実際に取得/リンクする `esp-idf-lib/i2cdev` 2.1.2 は MIT、source https://github.com/esp-idf-lib/i2cdev （manifest の `repository_info.commit_sha`: `7113b661fa467ad266e0b97b4e649ffa23d18cc5`）。ESP-IDF 5.4.3 は Espressif の SDK。依存パッケージや SDK は変更・同梱せず、Component Manager が例の `managed_components/` に取得したファイルは Git 対象外。
