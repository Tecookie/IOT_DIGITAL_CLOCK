import React from "react";
import { Card, Switch, InputNumber, Button, Badge, Row, Col, Typography, Divider } from "antd";
import { WiDayCloudy, WiHumidity, WiRaindrop } from "react-icons/wi";
import { AiOutlineWifi, AiOutlineClockCircle, AiOutlineFire } from "react-icons/ai";

const { Title, Text } = Typography;

export default function DisplayPanel({
  displayMode,
  status,
  setStatus,
  updateAlarm,
  countdownSeconds,
  setCountdownSeconds,
  startCountdown,
}) {
  const cardStyle = {
    borderRadius: 20,
    width: 360,
    minHeight: 220,
    padding: 24,
    boxShadow: "0 8px 16px rgba(0,0,0,0.1)",
    transition: "all 0.3s",
  };

  const formatData = (val, unit) => (val !== undefined ? `${Math.round(val)}${unit}` : "N/A");

  return (
    <div className="flex flex-wrap gap-6 justify-center">
      <Card style={cardStyle} hoverable className="transition-transform hover:scale-105">
        <Title level={3} className="text-blue-600 mb-5 text-center">
          {displayMode}
        </Title>

        {/* Connection Status */}
        {displayMode === "Connection Status" && (
          <div className="space-y-4">
            <Row align="middle">
              <AiOutlineWifi style={{ fontSize: 28, marginRight: 10, color: "#1890ff" }} />
              <Text strong style={{ fontSize: 18 }}>IP: {status.ip || "N/A"}</Text>
            </Row>
            <Row align="middle">
              <Badge
                status={status.wifiConnected ? "success" : "error"}
                text={
                  <Text strong style={{ fontSize: 18 }}>
                    {status.wifiConnected ? "Connected" : "Disconnected"}
                  </Text>
                }
              />
            </Row>
          </div>
        )}

        {/* Temperature / Humidity */}
        {displayMode === "Temperature" && (
          <div className="space-y-4 text-center">
            <Row align="middle" justify="center" className="mb-2">
              <AiOutlineFire
                style={{
                  fontSize: 40,
                  marginRight: 10,
                  color: status.tempAlert ? "#ff0000" : "#ff4d4f",
                }}
              />
              <Text
                style={{
                  fontSize: 32,
                  fontWeight: 700,
                  color: status.tempAlert ? "#ff0000" : "#222",
                }}
              >
                {formatData(status.temperature, "°C").replace("�", "°")}
              </Text>
            </Row>

            <Row align="middle" justify="center">
              <WiHumidity style={{ fontSize: 28, marginRight: 10, color: "#40a9ff" }} />
              <Text style={{ fontSize: 24, fontWeight: 700 }}>
                {formatData(status.humidity, "%")}
              </Text>
            </Row>

            {/* Show range + alert message */}
            <div className="mt-3">
              <Text type="secondary">
                Safe range: {status.tempLow ?? "--"}°C – {status.tempHigh ?? "--"}°C
              </Text>
              {status.tempAlert && (
                <div className="mt-2">
                  <Badge
                    status="error"
                    text={
                      <Text strong style={{ color: "#ff4d4f" }}>
                        ⚠️ Temperature out of range!
                      </Text>
                    }
                  />
                </div>
              )}
            </div>
          </div>
        )}
        {/* Weather */}
        {displayMode === "Weather" && (
          <div className="space-y-3">
            <Row align="middle" gutter={16}>
              <Col span={12} className="flex items-center gap-2">
                <WiDayCloudy style={{ fontSize: 40, color: "#1890ff" }} />
                <Text strong>{status.weatherDesc?.toUpperCase() || "N/A"}</Text>
              </Col>
              <Col span={12} className="flex items-center justify-end">
                <Text style={{ fontSize: 20 }}>{status.weatherTemp?.replace("�", "°") || "--°C"}</Text>
              </Col>
            </Row>
          </div>
        )}

        {/* Time */}
        {displayMode === "Time" && (
          <div className="space-y-2 text-center">
            <AiOutlineClockCircle style={{ fontSize: 40, color: "#1890ff" }} className="mx-auto" />
            <Text style={{ fontSize: 28, fontWeight: 700 }}>{status.time || "--:--:--"}</Text>
            <Text style={{ fontSize: 18, color: "#555" }}>📅 {status.date || "--/--/----"}</Text>
          </div>
        )}

        {/* Alarm */}
        {displayMode === "Alarm" && (
          <div className="space-y-4">
            <Row gutter={16} align="middle" justify="center">
              <Col>
                <Text strong>Hour:</Text>
                <InputNumber
                  min={0}
                  max={23}
                  value={status.alarmHour ?? 7}
                  onChange={(val) => setStatus({ ...status, alarmHour: val })}
                  style={{ marginLeft: 8 }}
                />
              </Col>
              <Col>
                <Text strong>Minute:</Text>
                <InputNumber
                  min={0}
                  max={59}
                  value={status.alarmMin ?? 0}
                  onChange={(val) => setStatus({ ...status, alarmMin: val })}
                  style={{ marginLeft: 8 }}
                />
              </Col>
              <Col>
                <Text strong>ON:</Text>
                <Switch
                  checked={status.alarmOn || false}
                  onChange={(val) => setStatus({ ...status, alarmOn: val })}
                  style={{ marginLeft: 8 }}
                />
              </Col>
            </Row>
            <Button type="primary" onClick={updateAlarm} block>
              Update Alarm
            </Button>
          </div>
        )}

        {/* Countdown */}
        {displayMode === "Countdown" && (
          <div className="space-y-4">
            <Text strong className="block">
              Select Preset Time:
            </Text>

            <select
              className="p-2 border rounded-lg w-full"
              onChange={(e) => {
                const val = e.target.value;
                if (val === "custom") return;

                const seconds = Number(val);
                setCountdownSeconds(seconds);
              }}
            >
              <option value="custom">⏱ Custom</option>
              <option value={180}>🍜 Instant Noodles (3 min)</option>
              <option value={300}>🥚 Boiled Egg Soft (5 min)</option>
              <option value={420}>🍳 Boiled Egg Medium (7 min)</option>
              <option value={600}>🍰 Baking Preheat (10 min)</option>
              <option value={900}>🔥 Grill Timer (15 min)</option>
            </select>

            <Divider />

            <Text strong className="block mb-1">
              Custom Time (HH : MM : SS)
            </Text>
            <div className="flex gap-3">
              <InputNumber
                min={0}
                max={23}
                placeholder="HH"
                onChange={(h) => {
                  const sec = (h ?? 0) * 3600 + (minutes ?? 0) * 60 + (seconds ?? 0);
                  setCountdownSeconds(sec);
                }}
              />
              <InputNumber
                min={0}
                max={59}
                placeholder="MM"
                onChange={(m) => {
                  const sec = (hours ?? 0) * 3600 + (m ?? 0) * 60 + (seconds ?? 0);
                  setCountdownSeconds(sec);
                }}
              />
              <InputNumber
                min={1}
                max={59}
                placeholder="SS"
                onChange={(s) => {
                  const sec = (hours ?? 0) * 3600 + (minutes ?? 0) * 60 + (s ?? 0);
                  setCountdownSeconds(sec);
                }}
              />
            </div>

            <Button type="primary" onClick={startCountdown} block>
              Start Countdown
            </Button>
          </div>
        )}      </Card>
    </div>
  );
}
