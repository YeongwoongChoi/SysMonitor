# SysMonitor
# 개요
- 이 프로그램은 서버와 클라이언트 간의 UDP 통신 방식을 이용한다.
- 클라이언트가 서버의 CPU, 메모리, 디스크 자원 사용률을 요청하면, 서버는 요청을 했던 클라이언트에게 자신의 사용률을 응답으로 제공한다.
- 이 문서에서 사용하는 shell 명령어는 Ubuntu 기준을 따른다.
        
# 빌드
## 빌드 전 준비 사항
- 서버에서 사용하는 헤더 (`src/server.h`)에 정의된 `PORT_NUM`에 맞추어 서버의 UDP 포트를 개방해야 한다.
    - 기본으로 사용하는 포트는 8080번이다.
        ```bash
        sudo ufw allow 8080/udp
        ```
- 빌드 시 `gcc`, `make`가 필요하므로, 설치가 안되어 있다면 설치한다.
    - 설치 방법
        ```bash
        # gcc, g++, make를 한 번에 설치한다.
        sudo apt install build-essential
        ```
    - 정상 설치 여부는 다음 명령어를 통하여 확인한다.
        ```bash
        gcc --version
        make --version
        ```

## 빌드 방법
### 디렉토리 구조
```bash
├── bin
│   ├── lib ── libresource.a (사용량 계산을 위한 정적 라이브러리)
│   ├── modules (서버에서 사용량을 응답하기 위해 사용하는 모듈)
│   │   ├── cpu.so
│   │   ├── mem.so
│   │   └── disk.so
│   ├── obj
│   └── server
├── src
│   ├── modules (사용량 응답 방식을 구체적으로 정의한 소스)
│   │   ├── cpu_module.c
│   │   ├── mem_module.c
│   │   └── disk_module.c
│   ├── common.h (서버 및 모듈에서 공통으로 사용하는 매크로)
│   ├── module.h (모듈 구조를 정의)
│   ├── resource.h (자원 사용량의 실제 값을 갖는 구조체, 값 계산 함수 정의)
│   ├── server.h (서버의 실제 응답 처리 로직 정의)
│   ├── resource.c
│   └── server.c
└── Makefile
``` 

- 프로젝트 루트에 있는 `Makefile`이 보일 것이다.
- `Makefile`을 이용하여 서버 바이너리와 모듈을 빌드할 수 있으며, 사용량 계산에 이용되는 함수들을 묶어둔 정적 라이브러리를 링크할 수 있다.
- 현재 `Makefile` 구조상으로 사용 가능한 명령은 다음과 같다.
    - `make`: 기본 동작으로, 서버 바이너리와 각종 서버 자원의 사용량을 응답할 모듈을 빌드한다.
    - `make server`: 서버 바이너리만 빌드한다.
    - `make modules`: CPU, 메모리, 디스크 사용량을 응답할 모듈들을 모두 빌드한다.
    - `make clean`: `bin` 하의 모든 빌드 결과물들을 삭제한다.
---

### Makefile 동작 과정 상세 설명
사용자가 터미널에 `make` 라고 입력하면, Makefile은 맨 위에 있는 첫 번째 규칙인 `all`을 최종 목표로 삼고 작업을 시작한다.

#### 변수 설정
- 컴파일러와 컴파일러 옵션을 설정한다.
- 동적 로드를 위한 `dlopen`, `dlsym` 함수 사용을 위해 링킹 시에 `-ldl` 옵션을 사용한다.
- 공유 라이브러리 생성 시 필요한 `-shared`, `-fPIC`를 사용한다.

#### **시작점: `all` 목표**
- `all: server modules`
- `make`는 `all`을 만들기 위해 두 가지 의존성인 `server`와 `modules` 타켓에 의존한다. 
- `make`는 이 의존성들을 순서대로 해결하기 시작한다.
---

#### **1단계: `server` 타겟**
- `make`는 `bin/server`를 만들기 위해 `server: $(SERVER_SRCS) $(STATIC_LIB)` 규칙을 확인한다.

* **목표:** `bin/server`
* **의존성:** `bin/server.c`와 `bin/libresource.a`
* **동작**
    - 우선 $(STATIC_LIB)가 최신 상태인지 확인한다.
    - 만약 `bin/lib/libresource.a` 파일이 없거나, 소스 파일(`src/resource.c`)보다 오래되었다면, 아래의 `$(STATIC_LIB)` 타겟을 먼저 실행한다.
    - `$(STATIC_LIB)`가 준비되면, `gcc`를 실행하여 `server` 실행 파일을 생성한다.
        - `-o $(BIN_DIR)/server`: `bin/server` 라는 이름의 실행 파일을 만든다.
        - `-L$(LIB_DIR)`: `bin/lib` 디렉토리를 라이브러리 검색 경로로 추가한다.
        - `-lresource`: `libresource.a` 라이브러리를 링크한다.

#### **`$(STATIC_LIB)` 타겟**
정적 라이브러리를 생성한다.
- **의존성:** `bin/obj/resource.o` 파일 (`$(COMMON_OBJ)` 타겟)에 의존
- **동작**
    - 우선 `$(COMMON_OBJ)`가 최신인지 확인하고, 필요하면 아래의 `$(COMMON_OBJ)` 타겟을 먼저 실행하여 `resource.o` 파일을 생성한다.
    - `ar rcs` 를 통해 `resource.o` 파일을 묶어 `libresource.a`를 생성한다.
---

#### **2단계: `modules` 타겟**
- **의존성:** `bin/modules` 하의 각 `.so` 파일에 의존
- **동작:** 각 `.so` 파일이 최신 상태인지 확인하고, 그렇지 않다면 아래의 패턴 규칙 (`$(MODULES_DEST_DIR)/%.so`) 타겟을 실행하여 파일을 생성한다.

#### **`$(MODULES_DEST_DIR)/%.so` 타겟**
`$(MODULES_DEST_DIR)`로 전체 모듈 소스에 대해 `.so` 파일을 생성한다. <br/>
아래는 `cpu.so` 파일이 생성되는 과정으로, `mem.so`, `disk.so`도 동일한 동작 과정을 거친다.
- **의존성:** `src/modules/cpu_module.c`와 `bin/lib/libresource.a`
- **동작**
    - `$(STATIC_LIB)`가 최신인지 확인하고, 필요하면 먼저 해당 타겟에서 생성한다.
    - `gcc` 를 실행하여 `cpu_module.c` 소스를 컴파일하고, `libresource.a`와 링크하여 `cpu.so` 공유 라이브러리 파일을 생성한다.
        - `$@`: 타겟 이름 (`bin/modules/cpu.so`)
        - `$<`: 첫 번째 의존성 파일 이름 (`src/modules/cpu_module.c`)
---

#### **3단계: `make clean`**
사용자가 `make clean`을 실행하면, `make`는 `clean` 규칙을 찾아 다음 명령을 실행한다.
* `rm -rf bin`
* 이 명령은 빌드 과정에서 생성된 모든 결과물(`bin` 디렉터리 전체)을 삭제하여, 처음부터 다시 빌드할 수 있는 상태로 만든다.
---
# 실행
- 클라이언트와 서버는 동일한 네트워크에 접속 (내부 IP를 통한 통신)
## 클라이언트
- `nc`(netcat)를 이용하여 서버에게 UDP 요청을 전송한다.
    ```bash
    # nc -u ${server_addr} ${port}
    nc -u 172.27.7.152 8080
    ```
- `nc`가 설치되지 않은 경우 다음 명령어를 이용하여 설치한다.
    ```bash
    sudo apt install nc
    ```
- 클라이언트가 요청할 수 있는 자원은 다음과 같다.
    - `cpu`: 서버의 전체 코어, 각 코어별 사용률을 요청한다.
    - `mem`: 서버의 전체 메모리 용량과 `free`, `used` 등 각 영역에 대한 사용량, 사용률을 요청한다.
    - `disk`: 서버의 전체 디스크 파티션별 사용량 및 사용률을 요청한다.
    * 이 외의 요청을 할 경우, 서버 측에서는 오류 메시지를 응답하며, 해당 요청은 무시된다.
## 서버
- `./bin/server`를 실행하여 클라이언트의 요청을 기다린다.
- 가령 `~/sysmonitor` 디렉토리 하에 설치되었다고 하면,
    ```bash
    cd ~/sysmonitor
    ./bin/server
    ```
- 클라이언트의 요청이 `cpu`, `mem`, `disk` 중 하나인 경우, 서버에는 표준 출력(`stdout`)으로 다음 형식의 내용을 남긴다.
    ```
    Sent 710 bytes to client(172.17.0.1) for request 'cpu'.
    ```

---
# 이 프로그램의 구조
서버 프로그램은 `bin/server` 바이너리를 이용하며, `server.c`에 서버의 동작이 구현되어 있다.

## 프로그램의 주요 함수

## `main`
- 서버 프로그램이 시작되면 `main` 함수에 진입한다.
- `initialize_server` 함수에서 이 서버 프로그램의 구성 요소를 초기화한다.
    - 프로그램이 Ctrl+C (`SIGINT`) 에 의해 종료될 경우, 기존에 사용하던 모듈들을 `cleanup_modules` 함수에서 close 하도록 구현
    - UDP 통신을 위한 소켓을 제공받고, 이를 주어진 포트를 이용하여 바인딩한다.

- 초기화 이후, `while` 루프 내에서 `process_request` 함수를 무한히 호출한다.
    - 클라이언트로부터 request를 수신하고, 수신한 request에 따라 해당 클라이언트에게 응답을 제공한다.
    - 반복된 `dlopen`, `dlclose` 호출을 방지하고자, 서버에는 각 자원을 반환하는 모듈에 대한 핸들러 정보를 가지고 있다.
        - 만약 모듈을 처음 사용하는 경우, 최초 1회 `dlopen`을 통하여 핸들러를 가져오고, 이를 캐시해둔다.
        - 이후 반복된 모듈 호출에서는, 캐시한 핸들러 정보를 이용하여 응답을 처리한다.
        - 프로그램이 종료될 경우, `cleanup_modules` 함수에서 모든 핸들러를 닫으며 UDP 소켓도 닫고 종료한다.

