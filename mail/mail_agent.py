import smtplib
import socket
import time
import os
import json
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText

_AGENT_IP = os.environ.get('AGENT_IP', '127.0.0.1')
_AGENT_PORT = int(os.environ.get('AGENT_PORT', '8080'))

print("[SysMonitor] Client started.")
print(f"[SysMonitor] Target agent -> {_AGENT_IP}:{_AGENT_PORT}")

def query_agent(resource_type):
	udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	udp_socket.settimeout(2.0)
	try:
		udp_socket.sendto(f"{resource_type}:json".encode(), (_AGENT_IP, _AGENT_PORT))
		data, _ = udp_socket.recvfrom(4096)
		return json.loads(data.decode())
	except Exception as e:
		print(f"[SysMonitor] Failed to query agent: {e}")
		return None

def send_mail(resource, amount, t):
	from_addr = 'noreply@localhost'
	to_addr = ['ywchoi@secuwow.com']

	msg = MIMEMultipart('alternative')
	msg['From'] = from_addr
	msg['To'] = ', '.join(to_addr)

	host = socket.gethostname()
	target_resource = resource.upper()
	msg['Subject'] = f"[SysMonitor Alert] {target_resource} 사용량 초과 ({host})"
	
	badge_color = "#e74c3c" if target_resource in ["CPU", "MEM"] else "#f39c12"
	body = f"""
	<html>
	<body style="font-family: 'Malgun Gothic', sans-serif; background-color: #f8f9fa; padding: 20px; color: #333333;">
        <div style="max-width: 550px; margin: 0 auto; background: #ffffff; border-radius: 8px; overflow: hidden; box-shadow: 0 4px 10px rgba(0,0,0,0.05); border: 1px solid #e9ecef;">
            <!-- 상단 바 -->
            <div style="background-color: {badge_color}; padding: 20px; text-align: center; color: #ffffff;">
                <h2 style="margin: 0; font-size: 22px; font-weight: 600;">System Alert Daemon</h2>
            </div>

            <!-- 본문 내용 -->
            <div style="padding: 30px 25px;">
                <p style="font-size: 16px; line-height: 1.6; margin-top: 0;">
                    호스트 <strong style="color: #2c3e50;">{host}</strong>의 서버 자원 임계치 장애가 감지되었습니다. 아래의 실시간 메트릭을 참조하여 즉시 조치를 취해주시기 바랍니다.
                </p>

                <!-- 메트릭 카드 블록 -->
                <div style="background-color: #f1f2f6; border-left: 5px solid {badge_color}; padding: 15px 20px; margin: 25px 0; border-radius: 0 6px 6px 0;">
                    <table style="width: 100%; border-collapse: collapse;">
                        <tr>
                            <td style="padding: 6px 0; font-size: 14px; color: #7f8c8d; width: 120px;">대상 리소스</td>
                            <td style="padding: 6px 0; font-size: 15px; font-weight: bold; color: #2c3e50;">{target_resource}</td>
                        </tr>
                        <tr>
                            <td style="padding: 6px 0; font-size: 14px; color: #7f8c8d;">실시간 점유율</td>
                            <td style="padding: 6px 0; font-size: 16px; font-weight: bold; color: {badge_color};">{amount:.2f}%</td>
                        </tr>
                        <tr>
                            <td style="padding: 6px 0; font-size: 14px; color: #7f8c8d;">탐지 시간</td>
                            <td style="padding: 6px 0; font-size: 14px; color: #2c3e50;">{t}</td>
                        </tr>
                    </table>
                </div>

                <p style="font-size: 14px; color: #95a5a6; margin-bottom: 0;">
                    * 본 메일은 SysMonitor 에이전트에 의해 자동 발송된 시스템 경고 메시지입니다.
                </p>
            </div>

            <!-- 하단 푸터 -->
            <div style="background-color: #fafafa; padding: 15px; text-align: center; font-size: 12px; color: #bdc3c7; border-top: 1px solid #eeeeee;">
                © 2026 SysMonitor. All rights reserved.
            </div>
        </div>
    </body>
    </html>
    """
	msg.attach(MIMEText(body, _subtype='html', _charset='utf-8'))

	try:
		with smtplib.SMTP(_AGENT_IP, 25) as server:
			server.sendmail(from_addr, to_addr, msg.as_string())
		print(f"[{t}] Notification Email sent.")
	except Exception as e:
		print(f"[{t}] Failed to send email: {e}")

while True:
	timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
	cpu_data = query_agent('cpu')
	mem_data = query_agent('mem')

	if cpu_data and "total" in cpu_data:
		cpu_usage = float(cpu_data['total'])
		if cpu_usage >= 70.0:
			send_mail("cpu", cpu_usage, timestamp)
			with open("monitoring.out", 'a') as f:
				f.write(f"Email sent at {timestamp}, CPU load: {cpu_usage:.2f}%\n")
	if mem_data and "used_percent" in mem_data:
		mem_usage = float(mem_data['used_percent'])
		if mem_usage >= 70.0:
			send_mail("mem", mem_usage, timestamp)
			with open("monitoring.out", 'a') as f:
				f.write(f"Email sent at {timestamp}, Mem usage: {mem_usage:.2f}%\n")

	time.sleep(10)
