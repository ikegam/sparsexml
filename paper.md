# SparseXML: 組み込みシステム向け軽量XMLパーサの評価

## 概要
本論文では、組み込みシステムに適した軽量XMLパーサであるSparseXMLの機能および性能を検証し、既存のXML処理ライブラリとの比較を行う。SparseXMLは1KBの固定バッファを利用することで最小限のメモリ使用量を実現し、イベント駆動型のSAXスタイルインタフェースを提供する。また、XMLは任意の位置で分割して逐次入力でき、W3CのEXI（Efficient XML Interchange）フォーマットについてもスキーマレスEXIのデコーダを備える。本研究では、関連研究の調査を行った上で、テストスイートを用いた実験による評価を実施した。

## 1. イントロダクション
近年、IoTや組み込みデバイスの増加に伴い、リソース制約の厳しい環境でもXMLデータを扱う需要が高まっている。従来のlibxml2 [21] などのフル機能XMLパーサはメモリ使用量やコードサイズが大きく、8ビットマイコンなどの小規模デバイスでは使用が難しい。SparseXMLはこの課題に対応するため、機能を最小限に絞りつつ、コメントやエンティティ処理など必要十分な機能を持たせたパーサとして開発された [6]。

本論文では、SparseXMLの設計方針と主要機能を解説し、関連研究との比較を通して本ライブラリの有用性を明らかにする。また、提供されているテストスイートを用いて正確性と拡張機能の動作を確認し、組み込み環境における利用可能性を評価する。

## 2. 関連研究
SparseXMLは、IoT機器の通信とXMLの交点に位置する。関連研究を、SparseXMLとの関係によって3つに分けて整理する。

### 2.1 IoTの通信規格におけるXMLとEXI
IoTの通信規格には、XMLやEXIでメッセージを表現するものが多い。これらは、SparseXMLが機器上で解析する対象である。

- **SenML（RFC 8428）** [12]: センサの測定値を表すIETFの形式で、共通のデータモデルをJSON、CBOR、XML、EXIの4つで表現できる。
- **IEEE 2030.5（Smart Energy Profile 2.0）** [13]: 電力機器向けのREST型プロトコルで、メッセージをXMLまたはEXIで符号化する（`application/sep+xml`、`application/sep-exi`）。
- **XMPPのIoT拡張**: XEP-0323〜0326がセンサデータや制御を、XEP-0322がEXIによる圧縮を定める [14]。ただし、XEP-0322は現在Deferredの状態である。
- **ISO 15118、DIN 70121**: 電気自動車と充電器の通信規格で、スキーマ情報付きEXIを使う。OpenV2G [10] やcbExiGen [11] は、これらのスキーマ専用のコーデックを生成する。

いずれの規格でも、メッセージの形式はスキーマで決まっている。SparseXMLが検証をホストに任せ、機器では決まった形式のメッセージを読むことに専念する設計（6.4節、6.5節）は、こうした用途を前提としている。

### 2.2 制約のある機器でのEXI処理
テキストのXMLは機器には重すぎるとして、EXIで解決しようとする研究がある。SparseXMLとは同じ問題に、EXIの側から取り組んでいる。

- **Käbischら（WEBIST 2012）** [15]: テキストのXMLは、解析の計算量とメモリ消費のためにマイコンでは使えないとしたうえで、EXIのデータをXPathの購読条件で絞り込む方式を示した。対象としたのは、24MHzのCortex-M3、RAM 8KB、フラッシュ128KBの機器である。
- **Kyusakovら（ACM Transactions on the Web, 2014）** [16]: 組み込み向けのEXI実装EXIP [9] を、組み込み機器でWebサービスを開発する枠組みとして示した。
- **Peintner（博士論文, 2014）** [17]: EXIficient [8] の作者で、組み込み機器でのEXIについて、実行時のメモリ使用を抑え、その増え方を予測する手法を論じた。
- **W3C「EXI Profile for limiting usage of dynamic memory」（2014年勧告）** [18]: センサやマイコンのような低メモリの機器向けに、EXIの動的メモリの使用を制限するプロファイルを定めた。SparseXMLがヒープを使わないことと、同じ問題意識に立つ。
- **佐藤・土井・寺本（情報処理学会論文誌 コンシューマ・デバイス&システム, 2012）** [19]: XMLを経由せずにEXIを直接扱う「XML-Less EXI」を提案し、ZigBee SEP 2.0のデータ形式に対応した家電用の通信アダプタを試作して評価した。

これらの研究は、テキストのXMLを機器で扱うことは難しいという前提に立つ。SparseXMLは、テキストのXMLも約1.2KBのRAMと約4.2KBのコードで解析できることを示し（5章）、さらにスキーマなしのEXIも同じインタフェースで扱える。

### 2.3 組み込み向けのXMLパーサ
SparseXMLと直接比較される、組み込み向けの軽量なパーサには次のようなものがある。

- **TinyXML [2]、TinyXML-2 [7]**: 小規模ながらDOMツリーを構築する方式をとり、メモリ使用量が文書の大きさに比例する。
- **Mini-XML** [3]: 通常のCライブラリに依存しない設計が特徴だが、DOMベースであるためデータ量に比例してメモリを消費する。
- **libexpat** [1]: ストリーミング型の実装で、メモリ効率は良いもののコードサイズが相対的に大きい。
- **saxml** [20]: ESP32などのマイコン向けのSAX型パーサで、文書を1文字ずつ受け取り、ヒープに文書を保持しない。

SparseXMLはSAXスタイルのイベント駆動型であり、DOMツリーを構築しないためメモリ使用量が非常に小さい。また、組み込みシステムで一般的に不足しがちなヒープ領域を一切使用せず、固定サイズのバッファのみで処理を完結させる点が他ライブラリと異なる。`sxml_init_explorer()`で呼び出し側が用意したメモリに解析器を配置でき、EXIの作業領域も`sxml_set_exi_workspace()`で静的バッファを渡せるため、ヒープが存在しないシステムや、動的メモリアロケーションを避ける必要がある組み込み環境でも利用可能である。

## 3. 設計
関連研究で挙げた各ライブラリはいずれも一長一短であり、組み込み用途で"そのまま"利
用するには課題が残る。TinyXMLやMini-XMLはDOMベースのため柔軟だが、データ量に比例
してヒープを消費し、コピペして使うにはソースが大きい。libexpatは高速かつ信頼性も
高いが、コードサイズが数十KB規模となり、ビルドや依存設定が煩雑である。

SparseXMLではこれらの問題点を解決するため、以下の設計方針を採った。

1. **ソースコードの単純化**: `sparsexml.h` を含むわずか3ファイルのみで構成し、外部
   依存を極力排した。必要に応じてプロジェクトへそのままコピーして利用できる。
2. **固定1KBバッファ**: 動的メモリ確保を行わず、構造体内の固定長バッファのみで解析す
   る。これにより、ヒープを持たないマイコンでも動作し、メモリ使用量が予測可能である。
3. **SAXスタイルのイベント駆動**: DOMツリーを構築せず、逐次コールバックで処理する
   ことでメモリフットプリントを最小化する。
4. **機能の選択的実装**: コメント、CDATA、エンティティ、名前空間、簡易DOCTYPEとい
   った基本機能に絞り、DTD検証や高度な名前空間URI解決などは省略した。スキーマに
   よる検証はデバイスでは行わず、開発環境（ホスト）で行う方針である（6.4節）。
5. **EXI（Efficient XML Interchange）サポート**: W3C標準のバイナリXMLフォーマット [4][5] に
   対応した。組み込みの文法（built-in grammar）と文法学習、文字列テーブルを実装し、
   EXIficientで生成したストリームをXMLと同じコールバックで受け取れる。EXIは文書全体を
   メモリに置いてデコードし、途中で切れたデータはエラーとして報告する。

こうした設計により、組み込み向けプロジェクトにおいてソースコードをそのまま取り込
み、最小限の構成で利用できる点が特徴となっている。

### 機能比較表
以下にSparseXMLと既存ライブラリの主な機能差を示す。SparseXMLで省かれている機能に
ついては、組み込み環境でのメモリ制約や実装規模を考慮した結果である。

| 機能                     | libexpat | TinyXML | Mini-XML | SparseXML | 省かれている理由 |
|-------------------------|:-------:|:------:|:-------:|:--------:|-----------------|
| DOM構築                 | ×       | ○      | ○       | ×        | メモリ使用を抑えるため |
| SAXスタイル             | ○       | ×      | ×       | ○        | - |
| コメント解析           | ○       | ○      | ○      | ○        | - |
| CDATA                   | ○       | ○      | ○       | ○        | - |
| 標準エンティティ       | ○       | ○      | ○       | ○        | - |
| 数値文字参照           | ○       | △      | △      | ○        | - |
| 拡張HTMLエンティティ   | ×       | ×      | ×       | ○        | 必要最低限の文字集合のみ実装 |
| 名前空間               | ○       | △      | △      | ○        | URI解決など高度機能を省略 |
| DOCTYPE解析           | ○       | ×      | △      | ○        | 検証処理は未実装 |
| EXI（バイナリXML）      | ×       | ×      | ×       | ○        | - |
| 部分データ処理（XML）   | △       | ×      | ×      | ○        | 任意位置で分割して入力可能 |
| DTD検証               | ○       | ×      | ×      | ×        | デバイスでは行わず、ホストで検証する方針 |

## 4. 実装
SparseXMLは`examples/simple.c`に示されるように、ユーザがコールバック関数を登録する形式で利用する。主なAPIは以下のとおりである。

```c
SXMLExplorer* sxml_make_explorer(void);
void sxml_destroy_explorer(SXMLExplorer* explorer);
// コールバックは通知元のexplorerと文字列を受け取る
typedef unsigned char (*SXMLCallback)(SXMLExplorer* explorer, char* text);
void sxml_register_func(SXMLExplorer* explorer,
                       SXMLCallback tag_func,
                       SXMLCallback content_func,
                       SXMLCallback attribute_key_func,
                       SXMLCallback attribute_value_func);
void sxml_set_user_data(SXMLExplorer* explorer, void* user_data);
void* sxml_get_user_data(SXMLExplorer* explorer);

// EXIサポート用のAPI（EXI用のexplorerを作って使う）
SXMLExplorer* sxml_make_exi_explorer(void);
void sxml_set_exi_options(SXMLExplorer* explorer, unsigned char options);
void sxml_set_exi_workspace(SXMLExplorer* explorer, void* buf, unsigned int size);
unsigned char sxml_run_explorer_exi(SXMLExplorer* explorer,
                                   unsigned char* exi,
                                   unsigned int len);
```

エンティティ処理や名前空間処理はオプションとして有効化でき、用途に応じて機能を絞ることが可能である。詳細なAPIはリポジトリの`README.md`で解説されている [6]。
本リポジトリにはベンチマークやテスト用の多数のファイルが含まれているが、ライブラリ本体は`sparsexml.h`、`sparsexml-priv.h`、`sparsexml.c`の3ファイルのみで構成されている。利用者が確認すべき公開インタフェースは`sparsexml.h`だけである。

## 5. 評価
### 5.1 テスト環境
評価はリポジトリに含まれる包括的な自動テストスイートを用いて行った。`make test`コマンドを実行すると、66件のテストが実行され、XMLコメント、CDATA、エンティティ処理、名前空間、分割入力、EXI処理など多岐にわたる機能を確認できる。EXIのテストでは、EXIficientで生成した`.exi`をデコードした結果が、元の`.xml`を解析した結果とイベント単位で一致することを確認している。

テストスイートは以下の7つのカテゴリに分類される：
1. **Core API & Basic Parsing**: 基本的なXML解析機能
2. **Internal Implementation**: 内部実装の正確性検証
3. **Advanced XML Features**: コメント、CDATA、エンティティなど高度機能
4. **Entity Processing**: 標準・数値・拡張エンティティ処理
5. **Real-world XML Samples**: 実世界のXML文書（Sitemap、Atom、RSS等）
6. **Chunked XML Parsing**: 分割されたXMLデータの処理
7. **EXI Support**: 元XMLとのイベント一致、フィデリティオプション、バイトアライン、途中切れや不正ヘッダの検出

実行結果はすべて成功であり、大きなXMLドキュメント（9,943バイト→4,113バイトEXI圧縮）を含む複雑なテストケースでも堅牢に動作することが確認された。

### 5.2 メモリ構造と使用量
SparseXMLは、ヒープに依存しない独特のメモリ構造を持つ。従来のXMLパーサが動的メモリアロケーション（malloc/free）を多用するのに対し、SparseXMLはXML解析中にヒープメモリを使用せず、SXMLExplorer構造体（x86-64で約1.2KB）内の固定長バッファのみで全ての処理を行う。EXIのデコードでは文字列テーブルと学習した文法を保持する作業領域（既定32KB、`sxml_set_exi_workspace()`で任意のバッファを指定可能）を別途使用する。

```
従来のXMLパーサ（DOM型）:
ヒープメモリ: XMLサイズに比例して増大
├── ノード1 (malloc)
├── ノード2 (malloc)
├── テキスト (malloc)
└── 属性配列 (malloc)

SparseXML（SAX型、ヒープ非依存）:
SXMLExplorer構造体 (約1KB):
├── パーサ状態 (数バイト)
├── 固定バッファ (1KB)
└── コールバック関数ポインタ (数バイト)
```

この設計により、XML文書のサイズに関係なく常に約1KBの固定メモリ使用量を実現している。一般的なDOM型パーサでは、XML文書のサイズに比例してメモリを消費するが、SparseXMLではイベント処理直後にデータを破棄できるためピーク使用量は常に一定である。

### 5.3 包括的ベンチマーク結果
ベンチマークスイート（`bench/`）により、様々なXMLパターンで性能を評価した。

- **測定環境**: Intel Core i7-6600U（2.6GHz）、Linux、gcc 16.2.1、`-O2`
- **比較対象**: libexpat 2.8.4、TinyXML-2 11.0.0（同じく`-O2`でビルド）
- **時間**: 3回実行した中央値。TinyXML-2は文字列からDOMを構築し終えるまでを計測した。DOM型パーサの利用者は、要素を読む前に必ずこのコストを払う。
- **メモリ**: SparseXMLはexplorerの大きさ。ExpatとTinyXML-2は、解析前後のヒープ使用量の差（`mallinfo2()`）またはパーサ本体の`malloc_usable_size()`。

以前の版の表では、TinyXMLの列はXMLを解析しないスタブを計測していた。このスタブは入力文字列を`malloc`して`memcpy`するだけだったため、TinyXMLが最も速く見え、メモリも構造体と文字列コピーの大きさ（basicで96バイト）しか計上されていなかった。本版では実物のTinyXML-2に置き換えて測り直した。

#### 基本性能比較（反復実行の合計時間）:
| ベンチマーク | 反復回数 | SparseXML | Expat | TinyXML-2 |
|-------------|--------:|----------:|------:|----------:|
| 基本XML（71バイト） | 100,000 | 0.040s | 0.271s | 0.117s |
| 混合コンテンツ | 50,000 | 0.428s | 1.214s | 0.649s |
| Unicode文字 | 30,000 | 0.201s | 0.491s | 0.250s |
| ストレス小（3階層×5要素） | 10,000 | 0.061s | 0.148s | 0.079s |
| ストレス中（5階層×8要素） | 5,000 | 0.091s | 0.220s | 0.124s |
| ストレス大（7階層×10要素） | 2,000 | 0.072s | 0.188s | 0.100s |

反復実行するすべての条件で、SparseXMLはExpatより2.4〜6.8倍、TinyXML-2より1.2〜2.9倍速かった。

#### メモリ使用量:
SparseXMLのメモリ使用量は、すべてのテストケースで約1.2KB（1,208〜1,224バイト）で一定であった。TinyXML-2は71バイトの文書でも16KBを確保し、28KBの文書（large_mem）では347KBと、文書の約12倍のヒープを使った。Expatは文書に応じて4KBから36KBまで変動した。

### 5.4 追加テスト
`bench/bench` にはベンチマークだけでなく簡単な検証用テストも備えている。XMLを1回だけ解析し、SparseXMLとExpatはタグ数や属性数を、TinyXML-2はDOMの要素数を確認した後にベンチマークを実行する形式だ。実行例を次に示す。

```
$ ./bench/bench 1000
[TEST] counters ok (tags=4, contents=2, attrs=1)
[TEST] expat counters ok (tags=4, contents=2, attrs=1)
[TEST] tinyxml2 ok (elements=2, heap=17280 bytes)
Benchmark    | Param    | SparseXML(bytes)   | Expat(bytes)   | TinyXML(bytes) | Sparse time(s)   | Expat time(s)    | Tiny time(s)
basic        |     1000 |               1208 |              0 |          16304 |         0.000381 |         0.002296 |         0.001067
```

この追加テストにより、単に速度を測定するだけでなく、各パーサが同じ文書を期待どおり解析していることも確認できる。TinyXML-2が文書を解析できなかった場合は、その理由を標準エラーに出力する。たとえば引数1000では、deep_nestingが1000階層の文書を作るため、TinyXML-2の上限（`TINYXML2_MAX_ELEMENT_DEPTH` = 500）を超えてエラーになる。

### 5.5 詳細ベンチマーク分析
ベンチマークスイートには、基本テストに加えて次の3つのカテゴリがある：

1. **Mixed Content**: 複雑な構造とテキストを含むXML（実世界のXML文書に近い）
2. **Unicode Content**: 多言語文字、絵文字、特殊記号を含むXML（国際化対応テスト）
3. **Stress Test**: 異なる深度とサイズでの負荷テスト（スケーラビリティ評価）

#### 全ベンチマーク結果（`-O2`、3回の中央値）:
| Benchmark    | Param    | SparseXML(bytes) | Expat(bytes) | TinyXML-2(bytes) | Sparse time(s) | Expat time(s) | TinyXML-2 time(s) |
|--------------|---------:|-----------------:|-------------:|-----------------:|---------------:|--------------:|------------------:|
| basic        |   100000 |             1208 |            — |            16304 |       0.039730 |      0.271188 |          0.116843 |
| large_mem    |     1000 |             1216 |        36016 |           347136 |       0.000139 |      0.000351 |          0.000358 |
| deep_nesting |      100 |             1216 |        20032 |            20176 |       0.000010 |      0.000023 |          0.000030 |
| many_attrs   |       50 |             1216 |         9600 |             8608 |       0.000002 |      0.000012 |          0.000011 |
| comments     |      100 |             1216 |         4256 |            17920 |       0.000010 |      0.000006 |          0.000006 |
| entities     |     1000 |             1216 |        18576 |            23168 |       0.000020 |      0.000007 |          0.000019 |
| mixed        |    50000 |             1208 |            — |            22192 |       0.427943 |      1.213576 |          0.649395 |
| unicode      |    30000 |             1208 |            — |            17856 |       0.201035 |      0.490760 |          0.249723 |
| stress_sm    |    10000 |             1208 |            — |            17856 |       0.061188 |      0.148430 |          0.078782 |
| stress_md    |     5000 |             1208 |            — |            37456 |       0.090974 |      0.219960 |          0.124063 |
| stress_lg    |     2000 |             1224 |            — |            58736 |       0.071906 |      0.187521 |          0.100156 |

「—」の行は、Expatのパーサ本体を`malloc_usable_size()`で測る方式であり、expat 2.8.4では0が返るため測定できなかった。large_memからentitiesまでの行は1回だけ解析した時間で、マイクロ秒単位の値には計測誤差が大きい。

#### 主要な発見:
1. **メモリ安定性**: SparseXMLは全ての条件でメモリ使用量が約1.2KBで一定。TinyXML-2は最小でも約8.6KBで、文書が大きくなるとそれに比例して増える
2. **速度**: 反復実行する条件では、Expatより2.4〜6.8倍、TinyXML-2より1.2〜2.9倍速い。1回だけの計測（comments、entities）ではExpatが速い場合もあるが、いずれもマイクロ秒単位の差である
3. **スケーラビリティ**: 文書の複雑さに関係なく安定した性能を維持
4. **Unicode対応**: 多言語文字を含む複雑なXMLでも高い処理性能を実現

## 6. 考察
SparseXMLの最大の利点は、メモリフットプリントの小ささと依存性の少なさである。イベント駆動型であるため、ユーザは必要な情報のみを逐次処理でき、不要なデータの保持を避けられる。

### 6.1 他の実装との比較
組み込み機器でXMLやEXIを扱う既存の実装と、SparseXMLを比較する。数値は5.3節の測定による。

#### XMLパーサとの比較
| 観点 | SparseXML | libexpat | TinyXML-2 |
|------|-----------|----------|-----------|
| 方式 | SAX（C） | SAX（C） | DOM（C++） |
| 解析中のRAM | 約1.2KBで一定 | 4〜36KB（5.3節） | 8.6〜347KB、文書の約12倍 |
| ヒープ | 不要（静的領域に配置可能） | 必要 | 必要 |
| コードサイズ | 約4.2KB（x86-64 `-Os`）、Cortex-M0で約2.9KB | 約170KB（配布版の共有ライブラリ、`-O2`） | 約31KB（x86-64 `-Os`）＋C++ランタイム |
| 分割入力 | 任意の位置で可 | 可 | 不可（文書全体が必要） |
| 速度（5.3節） | 基準 | 2.4〜6.8倍遅い | 1.2〜2.9倍遅い |
| 入れ子の深さ | 上限なし | 上限なし | 500階層まで |
| EXI | 同じコールバックでデコード | なし | なし |
| 組み込み方 | 3ファイルをコピー | ライブラリとして構築 | 2ファイル（C++） |

libexpatはSAX型で信頼性が高いが、コードサイズが一桁以上大きく、ヒープを前提とする。TinyXML-2は扱いやすいが、DOMのためRAMが文書の大きさに比例し、C++ランタイムが要る。SparseXMLは、MCUで制約になるRAM、ヒープ、コードサイズのすべてで最小であり、速度でも上回る。

#### EXI実装との比較
EXIの実装は、汎用だが大きいものと、小さいが特定のスキーマ専用のものに分かれる。

- **EXIficient、OpenEXI**: 仕様全体を実装しているが、JavaやC#で書かれており、MCUでは動かない。SparseXMLはテストデータの生成と結果の照合にEXIficientを使っている。
- **EXIP**: 組み込み向けのC実装で、スキーマ情報付きEXIにも対応する。ただし、最終更新は2014年で、アルファ版のままである。
- **OpenV2G、cbExiGen**: ISO 15118やDIN 70121のスキーマからCのコーデックを生成する。小さく高速だが、そのスキーマのメッセージしか扱えない。

SparseXMLは、スキーマに依存しない汎用のデコーダでありながら、XMLパーサに約5KBのコードを足すだけで済む。アプリケーションは、XMLとEXIを同じコールバックで受け取れる。デコード結果がEXIficientの入力XMLとイベント単位で一致することをテストで確認している（5.1節）。

#### 欠点と、それが実用上問題にならない理由
| 欠点 | 実用上の影響 |
|------|-------------|
| 文書を任意の順で参照・変更できない（DOMがない） | 組み込み機器が扱うメッセージは形式が決まっているので、必要な値をコールバックで取り出せば足りる。そのための状態はexplorerに置ける（6.3節）。 |
| スキーマ検証も整形式チェックもしない | 通信相手のメッセージ形式はファームウェアのビルド時点で決まっているので、検証はホストで行えばよい。libxml2を使ってホストで検証する方針である（6.4節、`FUTURE.md`）。 |
| 1,023バイトを超えるデータは切り詰められる | 上限は`SXMLElementLength`でビルド時に変えられる。メッセージ中の各値は通常これより十分短い。スキーマの`xs:maxLength`とビルド時に照合する仕組みを計画している（`FUTURE.md`）。ただし現状は、切り詰めが起きても通知されない。 |
| EXIはスキーマなし・圧縮なしのストリームのみで、ストリーム全体をメモリに置く必要がある | EXIの圧縮は大きな文書で効果を発揮するもので、小さなメッセージでは効果が小さい。小さなメッセージなら、全体をメモリに置いても負担は小さい。スキーマ情報付きEXIは、ホストで文法を生成する方針を検討している（`FUTURE.md`）。 |
| EXIのエンコーダがない | 受信専用である。機器からEXIで送信する必要がある用途では、別のエンコーダが要る。 |

最後の2つを除けば、欠点はどれも「デバイスでやる必要がないことを、デバイスでやっていない」というものである。メッセージ形式が決まっていて、RAM、ヒープ、コードサイズに制約がある機器にとって、SparseXMLの利点は大きく、欠点はほとんど問題にならない。

### 6.2 EXIサポートの意義
EXI（Efficient XML Interchange）フォーマットへの対応により、SparseXMLは単なるテキストXMLパーサーの枠を超えた。EXIは約58%の圧縮率を実現し、ネットワーク帯域の制約がある組み込み環境において大きな優位性を提供する。途中で切れたストリームや不正なストリームはエラーとして検出され、誤ったイベントを報告しない。

### 6.3 実装の最適化
XMLの状態機械は1文字単位で処理し、コメントやCDATAの終端判定に先読みを使わないため、入力をどの位置で分割しても同じイベント列が得られる。XMLとEXIのexplorerはどちらも共通部分（コールバック、オプション、通知用バッファ）を先頭に持つ構造体で、共通部分へのポインタとして同じ関数で扱える。解析状態はすべてexplorer内にあり、コールバックは引数で通知元のexplorerを受け取って`sxml_get_user_data()`で利用者データにたどり着けるため、グローバル変数を必要としない。コールバックは`SXMLCallback`型で登録するため、シグネチャの誤りはコンパイル時に検出される。以前は通知文字列からexplorerを逆算する関数を用意していたが、explorerを引数で渡す方式に改めた。呼び出し側のコード増加は数バイトで、逆算用の関数が不要になった分と相殺され、Cortex-M0（`-Os`）ではXML専用で2,866→2,858バイト、EXI込みで7,191→7,195バイトとほぼ変わらない。EXIデコーダはx86-64の`-Os`で約5KBのコードを追加するが、`SXML_NO_EXI`を定義すればXML専用の構成（約4.2KB）にできる。

### 6.4 制限事項と今後の展望
DTD検証や高度な名前空間URI処理など、複雑なXML仕様には対応していないため、利用範囲は限定される。XMLの整形式チェック（タグの対応など）も行わず、入力どおりにイベントを通知する。これらの検証は意図的にデバイスから外している。組み込み機器では通信相手のメッセージ形式がファームウェアのビルド時点で決まっているため、実行時に検証してもコードサイズとRAMに見合う利点が小さい。代わりに、`FUTURE.md`で提案しているとおり、通信相手のスキーマは開発環境で使う。テストデータのホスト上での検証、スキーマからの定数・表を含むヘッダの生成、スキーマ情報付きEXIの文法の事前生成を段階的に導入し、スキーマそのものはデバイスに載せない。EXIについては、ヘッダ内のオプション、圧縮・事前圧縮、スキーマ情報付きEXIには対応していない。しかし、IoTデバイスや組み込みシステムでは、このような機能よりもメモリ効率とコードサイズの最小化が優先されることが多い。今後、オプションとしてバッファサイズの可変化や、さらなるストリーミング性能の向上を図ることで、より幅広い応用が期待できる。

### 6.5 現代のMCU（組み込み）における位置づけ
現代のMCU（Cortex-M4/M7などで数十〜数百MHz、SRAMは数十〜数百KB）でXMLを扱う場面を想定し、SparseXML、libexpat、TinyXML-2の3つを観点ごとに比較する。数値は5.3節の測定による。

| 観点 | SparseXML | libexpat | TinyXML-2 |
|------|-----------|----------|-----------|
| 28KBの文書の解析時間（x86-64） | 0.14ms（約200MB/s） | 0.35ms（約80MB/s） | 0.36ms（約78MB/s） |
| 4KBのメッセージの解析時間（MCU、推定） | 約2ms | 約5ms | 約5ms |
| 解析中のRAM | 約1.2KBで一定 | 4〜36KB | 8.6〜347KB（文書の約12倍） |
| ヒープ | 不要 | 必要 | 必要 |
| 解析を始められる時点 | 断片を受信した時点 | 断片を受信した時点 | 文書全体を受信した後 |
| コードサイズ | 約4.2KB（Cortex-M0で約2.9KB）、C | 約170KB（配布版、`-O2`）、C | 約31KB、C++ランタイムが必要 |
| 入れ子の深さ | 上限なし | 上限なし | 500階層まで |

MCUでの解析時間は、x86-64より100倍遅いと仮定して見積もった値で、実機では測定していない。

#### 速度は決め手にならない
3つの中ではSparseXMLが最も速いが、その差はMCUでは問題にならない。MCUがこの環境より100倍遅いとしても、4KBのメッセージはどのパーサでも数ミリ秒以内に解析できる。一方、通信には桁違いの時間がかかる。たとえば115,200bpsのシリアル通信では、4KBを受信するだけで約0.36秒かかる。解析時間はどのパーサでも通信時間に埋もれる。

#### 決め手になるのはRAM、ヒープ、コードサイズ
- **RAM**: SparseXMLは文書の大きさにかかわらず約1.2KBで済む。libexpatは文書の構造に応じて数KB〜数十KBを使う。TinyXML-2は71バイトの文書でも16KBを確保し、28KBの文書には347KBを使った。SRAMが数十〜数百KBのMCUでは、TinyXML-2で扱える文書の大きさはRAMで制限される。
- **ヒープ**: SparseXMLは、explorerを静的領域に置けば解析中に`malloc`を呼ばない。そのため、メモリ不足やヒープの断片化で実行時に失敗することがなく、動的メモリ確保を避けるべき安全規格の下でも使える。libexpatとTinyXML-2は、どちらもヒープを前提とする。
- **受信バッファ**: SparseXMLとlibexpatは、受信した断片をそのまま渡せるので、受信バッファは小さくてよい。TinyXML-2は文書全体を受信してから解析するため、受信バッファとDOMの両方が要る。
- **コードサイズと依存関係**: SparseXMLはCで書かれた約4.2KBのコードで、依存ライブラリがない。libexpatは機能が多い分コードが一桁以上大きい。TinyXML-2は約31KBで、`new`/`delete`などC++ランタイムが要る。
- **入れ子の深さ**: SparseXMLのXML解析は入れ子の深さを記録しないため上限がなく、100,000階層の文書も解析できた。libexpatも同じ文書を解析できた。TinyXML-2は、再帰によるスタックあふれを防ぐため500階層までに制限している。EXIの入れ子の上限は`SXML_EXI_MAX_DEPTH`で指定する。

#### 使い分け
SparseXMLは、文書を任意の順で参照したり書き換えたりできない。必要な値は、利用者がコールバックで状態を持ちながら取り出す（その状態はexplorerの利用者データに置ける。6.3節）。

- **SparseXML**: メッセージの形式が決まっていて、RAM、ヒープ、コードサイズを節約したい機器向け。
- **libexpat**: 名前空間URIの解決やDTD内のエンティティ宣言など、XMLの仕様を広く扱う必要があり、数十KBのRAMとヒープを使える機器向け。
- **TinyXML-2**: RAMに余裕があり、文書を自由に参照・変更したい機器向け。

## 7. 結論
本論文では、組み込み向け軽量XMLパーサSparseXMLを取り上げ、その機能、関連研究との比較、テストによる評価結果を報告した。SparseXMLは1KBバッファのみで動作し、コメントやエンティティ処理など実用的な機能を備えている。さらに、W3C標準のスキーマレスEXIのデコードにより、バイナリXMLも同じインタフェースで扱えるようになった。

66件のテストによるテストスイートの結果から、基本的なXML処理から分割入力、EXI処理まで必要な機能が正しく実装されていることが確認できた。

今後はより大規模なベンチマークや実際の組み込みデバイス上での性能測定を通じて、さらなる最適化を検討していく。特にEXI処理のさらなる最適化や、5G/IoT環境でのリアルタイム処理性能の向上が期待される。

## 8. 参考文献
1. Clark, J., & Expat project. "Expat XML Parser." 1998. https://libexpat.github.io/
2. Thomason, L. "TinyXML." 2000. http://www.grinninglizard.com/tinyxml/
3. Sweet, M. R. "Mini-XML." 2003. https://www.msweet.org/mxml/
4. Schneider, J., Kamiya, T., Peintner, D., & Kyusakov, R. (Eds.). "Efficient XML Interchange (EXI) Format 1.0 (Second Edition)." W3C Recommendation, 11 February 2014. https://www.w3.org/TR/exi/
5. Peintner, D., & Pericas-Geertsen, S. (Eds.). "Efficient XML Interchange (EXI) Primer." W3C Working Draft, 24 April 2014. https://www.w3.org/TR/exi-primer/
6. Ikegami, H. "SparseXML: Minimal XML and EXI parser for microcontrollers." Git repository, 2013–. https://github.com/ikegam/sparsexml
7. Thomason, L. "TinyXML-2." https://github.com/leethomason/tinyxml2
8. Peintner, D. "EXIficient." https://github.com/EXIficient
9. Kyusakov, R. "EXIP: Embeddable EXI Processor in C." https://exip.sourceforge.net/
10. Siemens AG. "OpenV2G." ISO/IEC 15118 and DIN 70121 implementation in C. https://sourceforge.net/projects/openv2g/
11. chargebyte GmbH / EVerest. "cbExiGen." https://github.com/EVerest/cbexigen
12. Jennings, C., Shelby, Z., Arkko, J., Keränen, A., & Bormann, C. "Sensor Measurement Lists (SenML)." RFC 8428, IETF, 2018. https://doi.org/10.17487/RFC8428
13. IEEE. "IEEE Standard for Smart Energy Profile Application Protocol." IEEE Std 2030.5-2018 (superseded by IEEE Std 2030.5-2023). https://standards.ieee.org/ieee/2030.5/11216/
14. XMPP Standards Foundation. "XEP-0322: Efficient XML Interchange (EXI) Format." Version 0.6.0, 2018. https://xmpp.org/extensions/xep-0322.html
15. Käbisch, S., Kuntschke, R., Heuer, J., & Kosch, H. "Efficient Filtering of Binary XML in Resource Restricted Embedded Networks." Proc. WEBIST 2012, pp. 174–182, 2012. https://doi.org/10.5220/0003939001740182
16. Kyusakov, R., Puñal Pereira, P., Eliasson, J., & Delsing, J. "EXIP: A Framework for Embedded Web Development." ACM Transactions on the Web, 8(4), 2014. https://exip.sourceforge.net/publications.html
17. Peintner, D. "Efficient Exchange and Processing of Semi-structured Data in the Embedded Domain." Doctoral thesis, University of Passau, 2014. https://opus4.kobv.de/opus4-uni-passau/files/241/Peintner_Daniel.pdf
18. Fablet, Y., & Peintner, D. (Eds.). "Efficient XML Interchange (EXI) Profile for limiting usage of dynamic memory." W3C Recommendation, 9 September 2014. https://www.w3.org/TR/exi-profile/
19. 佐藤由美子, 土井裕介, 寺本圭一. "XML-Less EXIを搭載した家電用通信アダプタの試作および評価." 情報処理学会論文誌 コンシューマ・デバイス&システム, Vol.2, No.2, pp.54–62, 2012. https://ipsj.ixsq.nii.ac.jp/record/87776/files/IPSJ-TCDS0202007.pdf
20. zorxx. "saxml." ESP-IDF component, https://components.espressif.com/components/zorxx/saxml
21. Veillard, D., et al. "libxml2." https://gitlab.gnome.org/GNOME/libxml2
