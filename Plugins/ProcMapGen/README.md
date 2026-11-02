# ProcMapGen 사용 설명서

Unreal Engine 5.8용 절차적 맵 생성 + 레벨 빌드 플러그인.
격자 위에 방을 놓고 입구방에서 출구방까지 두 갈래 루트를 만든 뒤, 루트 옆에 방을 붙이고 방 타입과 에셋을 정해 실제 레벨로 스폰한다.
**같은 설정 + 같은 시드 = 항상 같은 맵**이다.

```
설정 에셋(UPMGSettings) + 시드
  → 맵 생성 (PMG::GenerateMap / Blueprint "Generate Map")   : 방 배치, 루트, 문, 방 타입, 에셋 배정 → FPMGResult (데이터만)
  → 레벨 빌더 (APMGMapBuilder)                               : 결과대로 방/문/벽/입구·출구 마커를 월드에 스폰
```

목차
1. [설치](#1-설치)
2. [빠른 시작](#2-빠른-시작)
3. [맵이 만들어지는 방식](#3-맵이-만들어지는-방식)
4. [설정 항목](#4-설정-항목)
5. [오류 처리 규칙](#5-오류-처리-규칙)
6. [설정 조정 가이드](#6-설정-조정-가이드)
7. [에셋 제작 규약](#7-에셋-제작-규약)
8. [방 타입 추가하기](#8-방-타입-추가하기)
9. [레벨 빌더](#9-레벨-빌더)
10. [사용 예시](#10-사용-예시)
11. [생성 결과 읽기](#11-생성-결과-읽기)
12. [에디터 도구](#12-에디터-도구)
13. [성능](#13-성능)
14. [알려진 한계](#14-알려진-한계)

---

## 1. 설치

1. 프로젝트 폴더에 `Plugins` 폴더가 없으면 만들고, `ProcMapGen` 폴더를 통째로 넣는다 (`<프로젝트>/Plugins/ProcMapGen/ProcMapGen.uplugin`).
2. 프로젝트를 연다. 패키징된 플러그인(`Binaries` 폴더 포함)은 Blueprint 전용 프로젝트에서도 바로 쓸 수 있다. 소스만 있는 플러그인은 C++ 프로젝트에서 빌드해야 한다.
3. **Edit → Plugins**에서 `ProcMapGen`(카테고리 Procedural)이 켜져 있는지 확인한다. 꺼져 있으면 켜고 에디터를 다시 시작한다.

모듈
- `ProcMapGen` (Runtime): 맵 생성, 레벨 빌더, Blueprint 노드
- `ProcMapGenTests` (Editor): 자동화 테스트. 게임 빌드에는 들어가지 않는다

---

## 2. 빠른 시작

**1) 설정 에셋 만들기**
1. Content Browser에서 우클릭 → **Miscellaneous → Data Asset** → 클래스 `PMGSettings` 선택 → 이름 예: `DA_MapSettings`.
2. 에셋을 열고 최소한 다음을 채운다.
   - **Assets → Standard Rooms → Single Rooms → Variants**에 항목 하나 이상 (1칸 방 에셋. 이 리스트가 비면 생성하지 않는다)
   - 항목마다 `Level`(방 레벨) 또는 `Actor Class`(방 Blueprint) **중 하나만** 지정
   - 필요하면 `Entrance Rooms`, `Exit Rooms`, `Doors`, `Walls`도 채운다 (비어 있으면 경고만 나오고 그 부분을 스폰하지 않음)
3. Details 패널 아래쪽 **Info → Validation Status**가 `OK`인지 본다. `Fail`이면 **Validation Messages**의 `[Error]` 줄을 고친다.

**2) 맵 띄우기**
1. 레벨을 열고 **Window → Place Actors** 검색창에 `PMG Map Builder`를 입력해 뷰포트로 끌어 놓는다. 빌더의 위치가 맵의 (0, 0) 셀 모서리이고, 맵은 +X(`Cols`), +Y(`Rows`) 쪽으로 펼쳐진다.
2. 빌더를 선택하고 Details 패널 **ProcMapGen → Settings**에 1)의 에셋을 지정한다.
3. 플레이(`Alt+P`). BeginPlay에서 설정 에셋의 `Seed`로 맵을 만든다. 플레이할 때마다 다른 맵을 원하면 빌더의 **Use Random Seed**를 켠다.
4. 플레이하지 않고 구조만 보려면 빌더 Details의 **Preview → Preview Layout** 버튼 ([12장](#12-에디터-도구)).

Output Log(**Window → Output Log**, 필터 `LogProcMapGen`)에 `스폰 완료 (Seed N, ...)` 또는 실패 사유가 나온다.

---

## 3. 맵이 만들어지는 방식

설정 항목의 뜻을 이해하는 데 필요한 만큼만 적는다.

```
+-+-+-+-+-+-+          E = 입구방, X = 출구방
|E#1#1#1#*|.|          1 = 루트 1, 2 = 루트 2
+#+-+-+#+-+-+          * = 추가방 (루트 옆에 붙인 방)
|2|*|.|1#1#X|          . = 미사용 방 (스폰하지 않거나 채움 방)
+#+#+-+-+-+#+          # = 문
|2#2#2#2#2#2|
+-+-+-+-+-+-+
```

1. 맵(`Rows` x `Cols` 칸)의 모든 칸을 1칸 방으로 시작한다.
2. **입구방과 출구방**을 `MinEntranceExitDistance` 이상 떨어뜨려 놓는다. 입구/출구의 크기(1칸/2칸/4칸)는 `Entrance Rooms`/`Exit Rooms`에 에셋이 있는 크기 중에서 고른다.
3. 입구 → 출구 **루트 1**(가장 짧은 길 중 하나)과, 루트 1을 피해 돌아가는 **루트 2**를 찾는다.
4. 루트 위의 칸을 합쳐 **큰 방**(2칸, 4칸)을 만든다. 루트가 큰 방을 관통한다. 루트에 다 만들지 못한 큰 방은 루트 바로 옆에 붙인다.
5. 루트의 이어진 방마다 **문**을 하나 단다.
6. 사용되는 방이 `TotalRoomCount`가 될 때까지 루트 방 옆의 빈 1칸 방을 **추가방**으로 붙인다 (문 하나). 추가방 뒤에 또 추가방을 붙이지는 않는다 (깊이 1).
7. **방 타입**: 입구방 `Entrance`, 출구방 `Exit`, 나머지는 `Standard`. 그다음 커스텀 타입(상점, 보물방 등)을 `Count`만큼 배정한다.
8. **에셋 배정**: 방마다 그 타입·크기 리스트에서 에셋과 회전을, 경계마다 문/벽 에셋을 고른다.
9. 어느 단계에서든 조건을 못 맞추면 그 시도는 버리고 다른 시드로 처음부터 다시 한다 (`MaxAttempts`번까지).

- 큰 방(`QuadRoomCount`, `DualRoomCount`)은 **모두 사용된다**. 사용되는 방의 크기별 수는 항상 4칸 = `QuadRoomCount`, 2칸 = `DualRoomCount`, 1칸 = `TotalRoomCount - QuadRoomCount - DualRoomCount`
- 루트 방(루트 1, 2에 든 방)과 추가방만 사용된다. 나머지는 미사용 방

---

## 4. 설정 항목

설정 에셋(`UPMGSettings`)의 Details 패널 순서대로.

### Map
| 항목 | 설명 |
|---|---|
| `Rows` | 월드 Y축 칸 수 (1~100) |
| `Cols` | 월드 X축 칸 수 (1~100) |
| `Seed` | 빌더가 BeginPlay에서 쓰는 기본 시드. 0도 보통 시드다 (무작위가 아님) |
| `Max Attempts` | 생성 최대 시도 횟수 (기본 50). 0 이하면 50 |
| `Cell Size` | 셀 1칸의 월드 크기 (cm, 기본 1000). 방 에셋 크기와 맞춘다 |

### Rooms
| 항목 | 설명 |
|---|---|
| `Quad Room Count` | 4칸(2x2) 방 수. 모두 사용됨. 입구/출구가 4칸이면 그 방도 포함 |
| `Dual Room Count` | 2칸 방 수 (가로/세로 합). 모두 사용됨 |
| `Total Room Count` | 사용되는 방의 총 수 (입구/출구, 큰 방 포함) |
| `Min Entrance Exit Distance` | 입구방~출구방 최소 거리. 셀 단위 방 중심 맨해튼 거리이며 루트의 방 수가 아니다. 큰 방이 많으면 루트에 자리를 남기려고 실제 거리는 더 커진다 (목표 거리 = max(이 값, 루트에 만들 큰 방 수), 맵 최대 거리 이하) |

### Route
| 항목 | 설명 |
|---|---|
| `Route Cost Penalty` | 루트 2를 찾을 때 루트 1의 방을 지나는 비용 (다른 방은 1, 기본 10). 클수록 루트 2가 루트 1에서 멀리 돌아간다. 1 이하면 2 |

### Assets
| 항목 | 설명 |
|---|---|
| `Entrance Rooms` / `Exit Rooms` / `Standard Rooms` | 기본 방 타입의 에셋. 각각 `Single Rooms`(1칸), `Dual Rooms`(2칸), `Quad Rooms`(4칸), `Doors`(이 타입 방으로 들어가는 문) 리스트 |
| `Custom Types` | 사용자 방 타입 ([8장](#8-방-타입-추가하기)) |
| `Doors` | 기본 문. 문 양쪽 방 타입에 `Doors`가 없으면 이 리스트를 씀. 모두 비어 있으면 개구부만 열림 |
| `Walls` | 벽(개구부 막이). 비어 있으면 연결되지 않은 개구부가 열린 채로 남음 |
| `Unused Room Policy` | 미사용 방: `DontSpawn`(스폰 안 함) / `SpawnFiller`(채움 에셋 스폰) |
| `Filler Rooms` | `SpawnFiller`일 때 미사용 방(1칸)에 쓰는 에셋 |
| `Entrance Marker Class` / `Exit Marker Class` | 입구방/출구방 중앙 바닥에 스폰할 액터 (예: `PlayerStart`). None이면 스폰 안 함 |

에셋 리스트의 항목(`Variants`의 원소)
| 항목 | 설명 |
|---|---|
| `Level` | 레벨 에셋. 동적 레벨 스트리밍으로 로드 |
| `Actor Class` | 액터(Blueprint) 클래스. 스폰 |
| `Allow Rotation` | 방 에셋만 보인다 (기본 켬). 켜면 90° 단위로 무작위 회전 (1칸/4칸 0~270°, 2칸 0/180°). 방향이 있는 라이팅/장식이 있으면 끈다. 문/벽은 회전하지 않는다 |

`Level`과 `Actor Class`는 **둘 중 하나만** 지정한다. 한 리스트 안에서 레벨 항목과 액터 항목을 섞어도 된다.

### Advanced (펼치기)
| 항목 | 설명 |
|---|---|
| `Spread Candidate Count` | 큰 방 자리를 고를 때 뽑는 후보 수 (기본 8). 클수록 큰 방끼리, 그리고 입구/출구에서 멀리 떨어지지만 시드마다 배치가 비슷해진다. 1 = 무작위 |

### Info (읽기 전용)
| 항목 | 설명 |
|---|---|
| `Single Room Count` | 큰 방을 모두 만든 뒤 맵에 남는 1칸 방 수 |
| `Used Single Room Count` | 사용되는 1칸 방 수 |
| `All Room Count` | 맵의 모든 방 수 |
| `Max Quad Room Count` / `Max Dual Room Count` | 큰 방 최대 수 |
| `Estimated Route Room Count` / `Recommended Total Room Count` | 루트에 필요한 방 수 추정과 권장 `TotalRoomCount` ([6장](#6-설정-조정-가이드)) |
| `Validation Status` / `Validation Messages` | 설정 검증 결과. 값을 바꿀 때마다 갱신된다 |

---

## 5. 오류 처리 규칙

설정이 잘못되었을 때 세 가지로 처리한다. 경고와 에러는 Output Log와 결과(`Warnings`, `FailReason`)에 함께 남는다.

**① 무시 (경고를 남기고 규칙대로 진행)**
| 상황 | 처리 |
|---|---|
| `TotalRoomCount` > 맵의 모든 방 수 | 모든 방 수로 보정 |
| 사용되는 방의 (타입, 크기) 리스트가 비어 있음 | 그 방은 스폰 생략 (구조는 그대로) |
| `Entrance Rooms`/`Exit Rooms`의 리스트 3개가 모두 비어 있음 | 1칸으로 보고 진행, 그 방은 스폰 생략 |
| 문에 쓸 리스트(양쪽 방 타입의 `Doors`, 전역 `Doors`)가 모두 비어 있음 | 문 없이 개구부만 열림 |
| `Walls`가 비어 있음 | 개구부가 열린 채로 남음 |
| `SpawnFiller`인데 `Filler Rooms`가 비어 있음 | 미사용 방에 아무것도 스폰하지 않음 |
| 항목의 `Level`과 `Actor Class`가 모두 비어 있음 | 그 항목이 배정된 곳은 스폰 생략 |
| 커스텀 타입 이름이 비어 있음 / 중복 / 예약어(`Entrance`, `Exit`, `Standard`) | 그 타입 무시 |
| `Route Cost Penalty` <= 1 | 2로 보정 |
| `Max Attempts` <= 0 | 50 |
| `TotalRoomCount`가 예산 추정보다 작음 | 그대로 진행 (실패할 수 있다는 경고와 권장값) |

**② 즉시 실패 (시도하지 않음, `Validation Status`가 `Fail`)**
- `Rows`/`Cols`가 1~100 밖, 큰 방 수가 음수
- 큰 방이 맵에 들어가지 않음: `QuadRoomCount > floor(Rows/2) * floor(Cols/2)`, `DualRoomCount > floor((Rows*Cols - 4*QuadRoomCount) / 2)`
- `TotalRoomCount < max(2, QuadRoomCount + DualRoomCount)`
- `MinEntranceExitDistance`가 맵 최대 거리 `(Rows-1) + (Cols-1)`보다 큼
- 입구/출구 크기 조합이 없음 (예: `Exit Rooms`에 4칸만 넣었는데 `QuadRoomCount` 0, 큰 입구/출구는 맵 안에서 그 거리만큼 떨어질 수 없음)
- `Standard Rooms`의 1칸 리스트가 비어 있음
- 쓰이는 리스트에 `Level`과 `Actor Class`를 모두 지정한 항목이 있음 (메시지에 리스트와 항목 번호)
- 커스텀 타입 `Count`가 그 타입이 될 수 있는 방 수보다 큼, 또는 `Count` 합이 `TotalRoomCount - 2`보다 큼

**③ 시도 실패 (다른 시드로 다시, `Max Attempts`번 모두 실패하면 생성 실패)**
| 단계 이름 (`FailReason`) | 뜻 |
|---|---|
| 입구/출구 지정 | 목표 거리 이상이고 겹치지 않는 입구/출구 자리가 없음 |
| 루트 찾기 | 루트 2가 루트 1과 같음 (한 줄 맵처럼 돌아갈 길이 없음) |
| 루트 위 큰 방 | 루트 방 + 루트 옆에 붙일 큰 방이 `TotalRoomCount`를 넘음 |
| 루트 옆 큰 방 | 루트 옆에 큰 방을 붙일 빈 자리가 없음 |
| 분기 채우기 | 루트 방 옆의 빈 방이 모자라 `TotalRoomCount`를 채우지 못함 |
| 방 타입 배정 | 커스텀 타입을 `Count`만큼 배정할 방이 없음 |

실패하면 `FailReason`이 이렇게 나온다:
```
시도 50회 모두 실패 (단계별: 루트 위 큰 방 12회, 분기 채우기 38회). 마지막 실패 (분기 채우기): 분기 채우기 자리 부족: ...
```
가장 많이 실패한 단계를 보고 [6장](#6-설정-조정-가이드)대로 설정을 고친다. 시도마다의 사유는 콘솔에 `log LogProcMapGen Verbose`를 입력하면 Output Log에 나온다.

---

## 6. 설정 조정 가이드

아래 숫자는 무작위 설정 1,300개(맵 1~100)로 생성을 돌려 본 결과(퍼즈 테스트)다. 대략적인 경향으로 본다.

### 6-1. 먼저 볼 것

설정 에셋의 **Info → Validation Status**가 `OK`인지, 그리고 `Recommended Total Room Count`(권장값). 생성이 실패하면 `FailReason`의 **단계별 실패 횟수**에서 가장 많은 단계를 찾아 아래 표대로 고친다.

| 가장 많이 실패한 단계 | 원인 | 고칠 것 |
|---|---|---|
| 루트 위 큰 방 | `TotalRoomCount`가 루트 방 + 큰 방을 담기에 작음 | `TotalRoomCount`를 늘린다 (6-2) |
| 루트 옆 큰 방 | 루트 옆에 큰 방을 붙일 빈 자리가 없음 (큰 방이 많음) | 큰 방 수를 줄이거나 맵을 키운다 (6-3) |
| 분기 채우기 | 루트 방 옆 빈 방이 모자람 (`TotalRoomCount`가 너무 큼) | `TotalRoomCount`를 줄이거나 `MinEntranceExitDistance`를 늘린다 (루트가 길어져 붙일 자리가 늘어남) |
| 방 타입 배정 | 커스텀 타입을 배정할 방이 모자람 | 6-5 |
| 루트 찾기 | 돌아갈 길이 없음 (폭 1~2칸인 좁은 맵) | 맵을 넓힌다 |

### 6-2. `TotalRoomCount` — 권장값의 2~3배

`Recommended Total Room Count`는 "루트에 필요한 방 수"의 추정인데, **실제로는 그 1.4배쯤(가운데 값)이 필요하다** (큰 방을 루트 위에 다 만들지 못해 루트 옆에 붙이는 등). 한편 추가방은 루트 방 옆에만 붙으므로 너무 크면 붙일 자리가 모자란다.

| `TotalRoomCount` / 권장값 | 생성 성공률 (큰 방 수 <= 맵 최대 거리인 설정) |
|---|---|
| 1 미만 | 0% (경고가 나옴) |
| 1 ~ 1.5 | 약 40% |
| 1.5 ~ 2 | 약 85% |
| 2 ~ 3 | 약 95% |
| 3 이상 | 표본이 적음. 시도 하나가 담을 수 있는 최대는 권장값의 약 4배(가운데 값), 낮게는 2~3배라 4배를 넘으면 분기 채우기 실패가 늘어난다 |

### 6-3. 큰 방 수 (`QuadRoomCount`, `DualRoomCount`)

- 큰 방은 루트 위에 만들고, 다 못 만들면 **루트 바로 옆**에 붙인다. 루트 길이는 입구~출구 거리쯤이라 큰 방을 담을 수 있는 양에 한계가 있다.
- **4칸 + 2칸 수를 맵 최대 거리 `(Rows - 1) + (Cols - 1)` 이하로** 둔다. 넘으면 성공률이 약 25%로 떨어진다 (대부분 "루트 옆 큰 방" 실패).
- 큰 방이 차지하는 칸(4칸 x 4 + 2칸 x 2)이 맵 칸의 **20% 이하**면 성공률 약 75%, 20~40%면 약 40%, 그 이상은 매우 낮다.
- 큰 방은 루트를 따라 몰려 나오기 쉽다 (큰 방의 약 90%가 다른 큰 방과 맞닿음). 더 흩어지게 하려면 `MinEntranceExitDistance`를 늘려 루트를 길게 하고, `Spread Candidate Count`를 키운다.

### 6-4. 루트 관련

- `MinEntranceExitDistance`: 클수록 루트가 길어져 루트 방이 늘고(→ 필요한 `TotalRoomCount`도 늘어남) 추가방을 붙일 자리도 늘어난다. 맵 최대 거리의 절반 정도에서 시작해 본다.
- `Route Cost Penalty`: 클수록 루트 2가 루트 1에서 멀리 돌아가 두 루트가 갈라진 맵이 된다 (루트 방 수가 늘어남). 작게 하면(2~3) 두 루트가 많이 겹친다.
- `Spread Candidate Count`(Advanced): 큰 방 자리를 고를 때 후보 수. 1 = 무작위, 8(기본) = 고르게 퍼짐, 16 = 더 퍼지지만 시드마다 배치가 비슷해진다.

### 6-5. 커스텀 타입

- 방 타입 배정에서 실패하는 시도는 그 단계까지 간 시도의 약 10%이고, `Avoid Door To Entrance/Exit`를 켠 설정은 약 25%다. 실패하면 다른 시드로 다시 하므로 `Max Attempts` 안에서 대개 성공한다.
- `Extra Room` 타입의 `Count`는 추가방 수(`TotalRoomCount` - 루트 방 수)보다 충분히 작게. 추가방이 몇 개 생길지는 시도마다 다르다.
- 1칸 리스트가 없는 `Extra Room` 타입(예: 4칸 보물방)은 큰 방을 예약해 루트 옆에 붙이므로, 큰 방 수(6-3)에도 영향을 준다.

### 6-6. `Max Attempts`

- 시도 하나는 100x100 맵에서 약 10~70 ms. 기본 50번이면 실패하는 설정도 몇 초 안에 끝난다.
- 성공률이 낮은 설정에서 늘리면 성공할 수는 있지만, 6-1 표대로 설정을 고치는 편이 낫다 (같은 시드라도 시도마다 다른 맵을 시도하므로 결과는 늘 같다).

---

## 7. 에셋 제작 규약

### 방 에셋 (방 레벨 또는 방 Blueprint)
| 규약 | 내용 |
|---|---|
| 크기 | 1칸 = `CellSize` x `CellSize`, 2칸 = `2*CellSize` x `CellSize`, 4칸 = `2*CellSize` x `2*CellSize` |
| 피벗 | **방 영역의 바닥 중앙**. 레벨 에셋은 레벨 원점(0, 0, 0)이 피벗 |
| 2칸 방 방향 | **가로(X축 2칸)로 만든다**. 세로 2칸 방에는 빌더가 90° 돌려 놓는다 |
| 회전 | `Allow Rotation`이 켜진 에셋만 추가로 돌린다 (1칸/4칸 0/90/180/270°, 2칸 0/180°) |
| 개구부 | 방 경계의 **모든 셀 엣지 중앙에 하나씩** 비워 둔다 (1칸 4개, 2칸 6개, 4칸 8개). 개구부 폭은 문/벽 에셋과 직접 맞춘다 |
| 벽 | 방 에셋에 포함. 개구부를 뺀 모든 경계를 채우고, **자기 영역 안쪽에만** 둔다 (맞닿은 두 방의 벽 두께가 합쳐짐). 런타임에 바꿀 수 없는 브러시 대신 스태틱 메시로 |
| 높이 | 방/문/벽/마커 모두 빌더 액터의 Z에 놓인다 (바닥 높이) |

방 레벨에 `PMG Room Guide` 액터를 놓으면 개구부 자리를 선으로 보여 준다 ([12장](#12-에디터-도구)).

### 문/벽 에셋 (커넥터)
| 규약 | 내용 |
|---|---|
| 단위 | 셀 경계(엣지) 하나에 액터 하나. 두 방의 벽 두께를 함께 채운다 |
| 피벗 | **개구부 바닥 중앙** |
| 방향 | **+X가 엣지에 수직**. 문은 +X가 `RoomA → RoomB`(입구 쪽 방 → 들어가는 방) 방향 |
| 문 | 문틀(개구부를 채우고 문 모양 구멍을 남긴 벽)과 문짝을 한 액터에 |
| 벽 | 개구부를 막는 판. 방향이 없는 문과 벽은 양면 대칭 권장 |
| 두께 | 합쳐진 벽 두께를 덮을 수 있게 |

### 레벨 에셋을 쓸 때
- 패키징한 게임에서 쓰려면 **Edit → Project Settings → Packaging → List of maps to include in a packaged build**에 방 레벨을 모두 넣는다 (레벨은 이름으로 불러오므로 쿠커가 자동으로 찾지 못함)
- 스트리밍 레벨 이름은 `<빌더 이름>_Room_<방 ID>`로 로드된다

### 방 Blueprint를 쓸 때
- 리플리케이트하지 않는 액터로 만든다 (`Replicates` 끔). 빌더는 각 머신에서 따로 스폰하므로 리플리케이트하면 서버/클라이언트에 중복된다
- 스폰 비용은 액터의 컴포넌트 수에 비례한다. 방이 많은 맵이면 메시를 합치거나(**Tools → Merge Actors**) Instanced Static Mesh로 컴포넌트 수를 줄인다 ([13장](#13-성능))

---

## 8. 방 타입 추가하기

**Assets → Custom Types**의 **+**로 타입을 추가한다.

| 항목 | 설명 |
|---|---|
| `Type Name` | 타입 이름 (예: `Shop`). 비어 있거나 중복이거나 `Entrance`/`Exit`/`Standard`면 무시 |
| `Count` | 이 타입으로 바꿀 방 수. 배정할 방이 모자라면 그 시도는 실패 (다른 시드로 다시) |
| `Extra Room` | 켜면 **추가방**(루트 옆에 붙인 막다른 방)에만 배정 |
| `Avoid Door To Entrance` / `Avoid Door To Exit` | 켜면 입구방/출구방과 **문으로 바로 이어진 방**에는 배정하지 않음 (예: 시작하자마자 들어가는 상점, 보스방에 딸린 보물방 막기). 벽만 맞닿은 방은 빼지 않는다 |
| `Assets` | 크기별 방 리스트(`Single/Dual/Quad Rooms`)와 이 타입 방으로 들어가는 문(`Doors`) |

**배정 규칙**
- 타입은 **에셋이 등록된 크기의 방에만** 배정된다. 예: 상점에 1칸 리스트만 있으면 상점은 1칸 방만 된다
- 대상은 사용되는 `Standard` 방 (입구/출구는 바뀌지 않음). `Standard Rooms`에 그 크기 에셋이 없는 방을 먼저 고른다 (스폰 생략되는 방을 줄이려고)
- 제약이 큰 타입부터 배정한다: 큰 방 예약 타입 → 등록된 크기 수가 적은 타입 → `Extra Room` 타입 → 문 제외 옵션을 많이 켠 타입 → 정의 순서
- `Extra Room`이 켜져 있고 1칸 리스트가 빈 타입(예: 4칸 보물방)은 `Count`만큼 큰 방을 **추가방용으로 예약**한다. 예약한 큰 방은 루트 위가 아니라 루트 옆에 붙는다

**타입별 문**
- 문 하나는 `RoomB`(들어가는 방) 타입의 `Doors` → `RoomA` 타입의 `Doors` → 전역 `Doors` 순서로 처음 비어 있지 않은 리스트를 쓴다
- 보통 문은 전역 `Doors`만 채우면 된다. 비밀방/보스방처럼 들어가는 문이 달라야 하는 타입에만 그 타입의 `Doors`를 채운다
- **열기 조건이 있는 문**(폭탄문, 열쇠문 등)은 `Extra Room` 타입에 쓰는 것을 권장한다. 추가방은 문이 하나뿐인 막다른 방이라 입구 → 출구 길을 막지 않는다. 루트 방이 될 수 있는 타입에 쓰면 길을 막을 수 있다 (플러그인은 검사하지 않음). 문을 여는 동작은 문 Blueprint가 맡는다

**예시**
| 하고 싶은 것 | 설정 |
|---|---|
| 상점 2개, 시작방 바로 옆은 안 됨 | `Shop`, Count 2, `Avoid Door To Entrance`, 1칸 리스트 |
| 4칸 보물방 1개, 막다른 방, 폭탄문 | `Treasure`, Count 1, `Extra Room`, 4칸 리스트, `Doors`에 폭탄문 |
| 보스방 = 출구 | 커스텀 타입이 아니라 `Exit Rooms`의 4칸 리스트에만 보스방 에셋 (출구가 4칸 방이 됨) |

---

## 9. 레벨 빌더

`APMGMapBuilder` (Place Actors 이름 `PMG Map Builder`)

| 항목 | 설명 |
|---|---|
| `Settings` | 설정 에셋 |
| `Generate On Begin Play` | 켜져 있으면 BeginPlay에서 생성 (기본 켬) |
| `Use Random Seed` | 켜져 있으면 BeginPlay에서 설정의 `Seed` 대신 무작위 시드. 쓴 시드는 결과의 `Seed`와 로그에 남는다 |
| `Spawn Time Budget Ms` | 한 프레임에 스폰하는 데 쓸 시간 (기본 10 ms). 넘으면 나머지는 다음 프레임에 (프레임마다 최소 1개). 처음 쓰는 액터 클래스는 스폰 전에 비동기로 불러온다. 0이면 `Generate`를 부른 프레임에 모두 스폰 (그 프레임이 멈춤) |
| `Generate(Seed)` | 지금 스폰한 것을 지우고 그 시드로 생성해 스폰을 시작 (Blueprint 호출 가능) |
| `Clear()` | 스폰한 액터를 지우고 레벨을 내린다 (레벨은 몇 프레임 뒤에 빠짐) |
| `On Generation Finished(Result, Spawn Errors)` | 완료 이벤트. 모두 스폰하고 레벨이 모두 로드된 뒤에 온다. 생성에 실패해도 온다 (`Result.Success` false). `Generate`를 부른 다음 프레임 이후에 오므로 BeginPlay에서 바인딩해도 놓치지 않는다 |
| `Get Build Progress(Completed Count, Total Count)` | 진행도 0~1 (로딩 바용). 전체 = 이번에 스폰할 방 + 문/벽 + 마커, 완료 = 스폰한 것(레벨은 로드되어 보이게 된 것, 실패한 것도 포함). 완료 뒤 1, `Clear` 뒤 0 |
| `Get Room Build Progress(Completed Room Count, Total Room Count)` | 방(사용된 방 + 채움 방)만 센 진행도. "방 120/225" 같은 글자용 |
| `Get Class Load Progress(Loaded Count, Requested Count)` | 스폰 전 액터 클래스 비동기 로드(`LoadingClasses`)의 진행도 0~1. 요청 수 = 이번에 불러온 클래스 수 (이미 메모리에 있던 클래스는 빠지므로 같은 세션에서 다시 생성하면 보통 0). 실패한 로드도 완료로 센다. 클래스 단위로 오른다. 불러올 것이 없으면 0/0이고 1. `Clear` 전까지 값이 남는다 |
| `Get Build State` | 지금 하는 일: `Idle`(생성 중 아님) / `WaitingForUnload`(앞 맵의 레벨이 빠지기를 기다림) / `LoadingClasses`(클래스 불러오는 중) / `Spawning` / `LoadingLevels`(방 레벨 로드 대기) |
| `Result`, `Spawn Errors`, `Spawned Actors`, `Spawned Levels` | 마지막 생성의 결과와 스폰한 것 (읽기 전용) |

- **진행도의 전체 수는 설정의 `TotalRoomCount`와 다르다**: 문/벽(방 수의 4~5배쯤), 채움 방, 마커가 더해지고, 에셋 리스트가 비어 스폰하지 않는 방은 빠진다. `Generate`를 부른 직후(맵 생성, 수십 ms 안)에 정해진다
- 문/벽이 방보다 훨씬 많으므로 로딩 바에는 `Get Build Progress`를 쓴다. 방 진행도만 쓰면 방을 다 스폰한 뒤 문/벽을 스폰하는 동안 바가 멈춰 있다
- `Get Build Progress`는 스폰을 시작해야 오른다. 처음 쓰는 클래스를 불러오는 동안(첫 실행에서 가장 오래 걸리는 구간)에도 바가 움직이게 하려면 `Get Class Load Progress`와 합친다 ([10장](#10-사용-예시))
- 프레임을 나눠 스폰하는 동안 게임은 계속 돈다. 맵이 다 만들어지기 전에 플레이어가 움직이지 않게 로딩 화면을 띄우고 완료 이벤트에서 닫는다 ([10장](#10-사용-예시))

- 맵의 (0, 0) 셀 모서리 = 빌더 위치. 빌더의 회전과 스케일은 쓰지 않는다
- 스폰 이름: `<빌더 이름>_Room_<방 ID>`, `_Connector_<커넥터 번호>`, `_EntranceMarker`, `_ExitMarker`. 같은 설정 + 같은 시드면 이름까지 같다
- 스폰 순서: 방(방 ID 순) → 문/벽 → 입구 마커 → 출구 마커. 프레임을 나눠도 순서와 이름은 같다
- **PlayerStart를 마커로 쓸 때**: BeginPlay에서 생성하면 플레이어는 이미 스폰된 뒤라 새 PlayerStart를 쓰지 않는다. 완료 이벤트에서 플레이어를 입구방으로 옮긴다 ([10장](#10-사용-예시))
- 스폰 문제(에셋 항목이 비어 있음, 클래스/레벨을 불러오지 못함)는 `Spawn Errors`와 경고 로그에 남고, 나머지는 그대로 스폰한다
- 에디터에서 플레이하지 않는 상태로 스폰하는 기능은 없다 (미리보기는 선으로만)

---

## 10. 사용 예시

**싱글플레이 — 레벨에 빌더만**
빌더를 놓고 `Settings`를 지정, `Generate On Begin Play` 켬. 매번 다른 맵이면 `Use Random Seed`도 켬.

**플레이어를 입구방으로 옮기기 (레벨 블루프린트)**
1. 레벨에 놓은 빌더를 선택한 채 툴바 **Blueprints → Open Level Blueprint** → 그래프에서 우클릭 → `Create a Reference to <빌더>`
2. `Event BeginPlay` → 빌더 레퍼런스에서 `Assign On Generation Finished`
3. 이벤트에서: `Result` → `Success`가 true이면 `Get Player Pawn` → `Set Actor Location`. 위치는 입구 마커(`Entrance Marker Class`를 지정했다면 `Spawned Actors`의 마지막에서 두 번째, 출구 마커도 지정했을 때)의 위치, 또는 `Result → Rooms[Entrance Room Id] → Bounds`의 중심 x `CellSize` + 빌더 위치

**로딩 화면과 진행 바 (UMG)**
1. Content Browser 우클릭 → **User Interface → Widget Blueprint**로 로딩 위젯을 만들고, **Progress Bar**와 **Text**를 놓는다.
2. 위젯에 빌더 레퍼런스 변수(타입 `PMG Map Builder`)를 만든다 (**Expose on Spawn** 켬).
3. Progress Bar의 **Percent** 옆 **Bind** → 함수에서 빌더 → `Get Build Progress`의 반환값을 돌려준다. Text는 `Get Room Build Progress`로 "방 {완료}/{전체}", 또는 `Get Build State`로 "불러오는 중 / 만드는 중".
   - 클래스를 불러오는 구간도 바에 넣으려면 두 진행도의 **개수**를 가중치를 두어 더한다. 클래스는 종류 수라 적고(예: 10개) 스폰은 개수라 많으므로(예: 1,000개) 그냥 더하면 불러오는 구간이 바의 1%밖에 안 된다. 클래스 하나를 스폰 몇 개로 칠지(`ClassLoadFactor`)를 정해:
     ```
     진행도 = (Loaded Count x ClassLoadFactor + Completed Count) / (Requested Count x ClassLoadFactor + Total Count)
     ```
     분모가 0이면(앞 맵의 레벨이 빠지기를 기다리는 동안) 0으로 둔다. 불러올 클래스가 없으면 `Requested Count`가 0이라 스폰 진행도만 남는다. `ClassLoadFactor`는 에디터를 새로 켠 첫 실행에서 바가 두 구간을 고르게 지나가도록 맞춘다 (예: 10에서 시작)
4. 레벨 블루프린트: 빌더의 `Generate On Begin Play`를 끄고, `Event BeginPlay` → `Create Widget`(빌더 연결) → `Add to Viewport` → 빌더 `Generate(Seed)`. 완료 이벤트(`Assign On Generation Finished`)에서 위젯 `Remove from Parent` 후 플레이어를 입구방으로 옮긴다.

`Spawn Time Budget Ms`를 0으로 두면 한 프레임에 모두 스폰하므로 그동안 위젯도 그려지지 않는다.

**시드를 직접 정하기**
빌더의 `Generate On Begin Play`를 끄고 원하는 때에 `Generate(Seed)`를 부른다. 다시 생성할 때도 `Generate`만 부르면 앞의 맵을 지우고 만든다.

**멀티플레이**
플러그인에는 네트워크 코드가 없다. 같은 설정 + 같은 시드면 어느 머신에서든 같은 맵이 나오는 것을 이용한다.
1. 빌더의 `Generate On Begin Play`를 끈다 (`Use Random Seed`도 끔. 머신마다 다른 시드를 고르게 됨)
2. 서버가 시드를 정해 리플리케이트 변수(예: GameState) 등으로 클라이언트에 보낸다
3. 서버와 각 클라이언트가 그 시드로 각자 `Generate(Seed)`를 부른다
4. 방/문/벽 Blueprint는 리플리케이트하지 않는 클래스로 만든다 ([7장](#7-에셋-제작-규약))

**데이터만 쓰기 (스폰은 직접)**
- Blueprint: 우클릭 → `Generate Map` (카테고리 ProcMapGen). `Settings`와 `Seed`를 넣으면 `PMGResult`가 나온다
- C++:
```cpp
#include "PMGGenerator.h"
#include "Data/PMGSettings.h"

const FPMGResult Result = PMG::GenerateMap(*Settings, Seed);
if (!Result.bSuccess)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Result.FailReason);
	return;
}
for (const FPMGResultRoom& Room : Result.Rooms) { /* Room.Bounds, Room.RoomType, Room.VariantIndex ... */ }
```
모듈의 `Build.cs`에 `PublicDependencyModuleNames.Add("ProcMapGen");`를 추가한다. 스폰 위치는 `PMG::GetRoomSpawnTransform()`, `PMG::GetConnectorSpawnTransform()`(`Build/PMGMapBuilder.h`)로 빌더와 같게 구할 수 있다.

---

## 11. 생성 결과 읽기

`FPMGResult`
| 필드 | 설명 |
|---|---|
| `Success`, `Fail Reason`, `Attempt Count`, `Seed` | 성공 여부, 실패 사유, 몇 번째 시도에서 끝났는지, 쓴 시드 |
| `Warnings` | 설정 검증 경고 + 에셋 배정 경고 (스폰 문제는 빌더의 `Spawn Errors`) |
| `Rooms` | 모든 방 (미사용 방 포함). 인덱스 = 방 ID. 방 ID는 방의 왼쪽 위 셀 기준 위쪽 줄부터, 왼쪽부터 |
| `Doors` | 연결된 방 쌍마다 문 하나 (`RoomA` = 입구 쪽, `RoomB` = 들어가는 방, `CellA`/`CellB` = 경계 양쪽 셀) |
| `Connectors` | 스폰되는 방이 접한 셀 경계마다 하나 (문 또는 벽) |
| `Entrance Room Id`, `Exit Room Id`, `Primary Route`, `Secondary Route` | 입구/출구 방 ID, 루트 1/2 (입구 → 출구 순서의 방 ID) |

`FPMGResultRoom`: `Id`, `Shape`(Single / DualHorizontal / DualVertical / Quad), `Bounds`(셀 범위 [Min, Max), X = 열 = 월드 X), `Room Type`, `Neighbors`, `Connected`(사용), `Extra`(추가방), `Variant Index`(배정된 에셋. 사용된 방은 그 타입·크기 리스트의 인덱스, 미사용 방은 `Filler Rooms`의 인덱스, -1이면 스폰 안 함), `Rotation`(0~3, x90°)

`FPMGResultConnector`: `CellA`(맵 안), `CellB`(맵 밖일 수 있음), `Type`(Door/Wall), `Door List Room Type`(문 에셋을 가져온 타입, None = 전역 `Doors`), `Variant Index`

---

## 12. 에디터 도구

**구조 미리보기** (빌더 Details → **Preview**)
- `Preview Layout`: 설정의 `Seed`로 생성해 구조를 선으로 그린다 (스폰 없음, 플레이할 때와 같은 맵)
- `Preview Random Seed`: 무작위 시드로 미리보기. 쓴 시드는 `Preview Seed`와 로그에. 마음에 들면 그 값을 설정의 `Seed`에 넣는다
- `Clear Preview`: 선 지우기
- 표시: 흰 테두리 = 사용된 방 (회색 = 미사용), 이중선 = 추가방, 흰 원 / 흰 X = 입구 / 출구, 색 상자 = 커스텀 타입 (`Custom Types` 순서대로 빨강, 초록, 파랑, 시안, 자홍, 노랑), 주황 화살표 = 문 (RoomA → RoomB), 분홍 / 하늘색 선 = 루트 1 / 2. 위에서 보려면 뷰포트 **Perspective → Top**

**방 에셋 가이드** (`PMG Room Guide` 액터)
- 방 레벨에 놓고 위치를 (0, 0, 0)(방 피벗)으로, `Settings`(CellSize), `Shape`, `Guide Height`를 정하면 방 영역과 개구부 자리(초록 세로선 + 방 밖 방향 화살표)를 그린다
- 에디터 전용 액터라 방 레벨에 남겨 두어도 게임에는 들어가지 않는다

---

## 13. 성능

개발 PC(16코어, Development 에디터, 그래픽 없이 실행)에서 잰 값이다. 대략적인 크기만 참고한다.

**맵 생성 (`GenerateMap`, 스폰 없음)** — 100x100 맵
| 설정 | 한 번 생성 |
|---|---|
| 1칸 방만, 사용되는 방 300개 | 약 11 ms |
| 큰 방 150개, 사용되는 방 225 ~ 450개 | 약 12 ms |
| 입구/출구 거리 198 (맵 모서리끼리) | 약 43 ms |
| 시도가 모두 실패하는 설정 (큰 방 900개) | 시도 하나 약 76 ms x 50번 = 약 3.8초 |

생성은 한 프레임 안에 끝나는 수준이라 비동기 생성은 두지 않았다. 시도가 계속 실패하는 설정은 `Max Attempts`번을 다 돌고 실패하므로 그만큼 오래 걸린다. `FailReason`을 보고 설정을 고친다.

**스폰** — 개발 프로젝트의 방 BP(스태틱 메시 여러 개) / 방 레벨, 미사용 방은 스폰하지 않음. 클래스를 이미 불러온 상태
| 맵 (사용되는 방) | 스폰 수 | `Spawn Time Budget Ms` 10 (기본) | 0 (한 번에) |
|---|---|---|---|
| 방 BP 10x10 (11개) | 액터 57개 | 가장 긴 프레임 약 19 ms, 5프레임 | 한 프레임 약 79 ms |
| 방 BP 30x30 (45개) | 액터 249개 | 약 40프레임 (프레임 대부분 10 ms대) | 한 프레임 약 0.5초 |
| 방 BP 100x100 (225개) | 액터 1,228개 | 약 190프레임, 가장 긴 프레임 약 45 ms | 한 프레임 약 2.6초 |
| 방 레벨 100x100 (225개) | 레벨 225개 + 문/벽 액터 1,002개 | `Generate()` 약 20 ms, 레벨 로드는 여러 프레임 | `Generate()` 약 15 ms (문/벽 액터만 그 프레임) |

- 액터 하나를 스폰하는 데 이 프로젝트의 방 BP는 약 2 ms, 문/벽은 그보다 가볍다. 시간 예산은 "한 프레임에 이만큼까지"라 스폰이 다 끝나는 데는 (액터 수 x 액터당 시간)보다 오래 걸린다 (프레임마다 게임의 다른 일도 하므로)
- 처음 쓰는 클래스를 불러오는 시간(이 프로젝트에서 수백 ms ~ 수 초)은 비동기라 프레임을 멈추지 않는다 (`Get Build State`가 `LoadingClasses`)
- 액터 하나가 시간 예산보다 무거우면 그 한 개를 스폰하는 프레임은 예산을 넘는다. 방 BP의 컴포넌트를 줄이거나(메시 합치기, Instanced Static Mesh) 방 레벨을 쓰면 가벼워진다
- **방 레벨**: 레벨 로드는 엔진이 비동기로 하고, 로드된 레벨을 월드에 붙이는(보이게 하는) 프레임에서 시간이 든다 (100x100에서 가끔 수백 ms 프레임). 완료 이벤트는 모든 레벨이 보인 뒤에 온다
- 문/벽은 항상 액터라 방 레벨을 써도 커넥터 수만큼 액터를 스폰한다
- 측정은 그래픽 없이 한 값이다. 렌더링 비용(액터가 많을수록 늘어남)은 들어 있지 않다

---

## 14. 알려진 한계

- 2D 한 층 맵만 만든다. 개구부는 모든 셀 엣지 중앙에 하나로 고정
- 추가방은 루트 방에만 붙는다 (깊이 1). 그래서 `TotalRoomCount`는 루트 방 수의 일정 배수를 넘을 수 없다 ([6장](#6-설정-조정-가이드))
- 루트 1은 거의 최단 경로라 계단 모양이 되기 쉽다
- 에셋 선택에 가중치가 없다 (리스트 안에서 고르게)
- 스폰을 프레임에 나눠도 무거운 액터 하나, 레벨을 월드에 붙이는 프레임은 길어질 수 있다 ([13장](#13-성능))
- 생성 결과를 레벨에 저장(베이크)하는 기능은 없다
