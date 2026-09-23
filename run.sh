#!/bin/bash

AGENT_PORT="${1:-8080}"
echo "[SysMonitor] $0 has started. (Designated port: ${AGENT_PORT})"

MAIL_AGENT="mail_agent.py"
SERVER_AGENT_PID="server_agent.pid"
MAIL_AGENT_PID="mail_agent.pid"

# ===============================================================
# 1. 기존 실행 중인 프로세스 종료 (PID 파일 활용)
# ===============================================================
if [ -f "${SERVER_AGENT_PID}" ]
then
	PID=$(< "${SERVER_AGENT_PID}")
	if ps -p "${PID}" > /dev/null 2>&1
	then
		echo "Stopping existing C agent (PID: ${PID})...."
		kill -9 "${PID}" 2>/dev/null
	fi
	rm -f "${SERVER_AGENT_PID}"
else
	# 예외 케이스: PID 파일은 없으나 포트가 선점되어 있을 때를 대비한 백업
	if command -v lsof >/dev/null 2>&1
	then
		PID=$(lsof -t -i udp:"${AGENT_PORT}" 2>/dev/null)
	elif command -v ss >/dev/null 2>&1
	then
		PID=$(ss -lupn "sport = :${AGENT_PORT}" 2>/dev/null | grep -oP 'pid=\K\d+' | head -n 1)
	fi

	if [ ! -z "${PID}" ]
	then
		PROC_NAME=$(ps -p "${PID}" -o comm= 2>/dev/null)
		if [[ "${PROC_NAME}" == *server ]]
		then
			echo "Found orphan C agent using port ${AGENT_PORT} (PID: ${PID}). Stopping it..."
			kill -9 "${PID}" 2>/dev/null
		fi
	fi
fi

if [ -f "${MAIL_AGENT_PID}" ]
then
	PID=$(< "${MAIL_AGENT_PID}")
	if ps -p "${PID}" > /dev/null 2>&1
	then
		echo "Stopping existing Mail agent (PID: ${PID})..."
		kill -9 "${PID}" 2>/dev/null
	fi
	rm -f "${MAIL_AGENT_PID}"
else
	# 예외 케이스: PID 파일은 없으나 구동 중일 때
	pkill -9 -f "${MAIL_AGENT}" 2>/dev/null
fi

# ===============================================================
# 2. agent 자원 query를 위한 포트 개방
# ===============================================================
PROTOCOL="${AGENT_PORT}/udp"

# Ubuntu/Debian
if command -v ufw > /dev/null
then
	if sudo ufw status | grep -q "${PROTOCOL}"
	then
		echo "[ufw] "${PROTOCOL}": already allowed"
	else
		echo "[ufw] "${PROTOCOL}" will be added"
		sudo ufw allow "${PROTOCOL}" > /dev/null
		sudo ufw reload > /dev/null
	fi
# Rocky/CentOS/RHEL
elif command -v firewall-cmd > /dev/null
then
	if sudo firewall-cmd --state > /dev/null 2>&1
	then
		if sudo firewall-cmd --list-ports | grep -q "${PROTOCOL}"
		then
			echo "[firewalld] "${PROTOCOL}": already allowed"
		else
			echo "[firewalld] "${PROTOCOL}" will be added"
			sudo firewall-cmd --permanent --add-port="${PROTOCOL}" > /dev/null
			sudo firewall-cmd --reload > /dev/null
		fi
	fi
fi

# ===============================================================
# 3. 프로그램 재빌드 여부 선택 및 빌드 
# ===============================================================
read -p "Setup completed. Do you want rebuild program? [y/n] > " s

if [[ "$s" == y ]]
then
	cd agent
	make clean && make
	cd ..
fi

# ===============================================================
# 4. 메일 발송 agent 및 자원 agent 백그라운드 구동 및 로그 세팅, PID 기록
# ===============================================================
./agent/bin/server ${AGENT_PORT} > agent.log 2>&1 &
echo $! > "${SERVER_AGENT_PID}"
echo "[SysMonitor] Agent started in the background (PID: $(< ${SERVER_AGENT_PID})), port: ${AGENT_PORT}"

python3 ./mail/${MAIL_AGENT} > mail.log 2>&1 &
echo $! > "${MAIL_AGENT_PID}"
echo "[SysMonitor] Mail agent started in the background (PID: $(< ${MAIL_AGENT_PID}))"