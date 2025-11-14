import React, { useEffect, useState } from "react";
import { Typography, Divider } from "antd";
import { AiOutlineClockCircle } from "react-icons/ai";
import ModeSelector from "./components/ModeSelector";
import DisplayPanel from "./components/DisplayPanel";
import {
  getStatus,
  updateAlarm,
  startCountdown,
  changeLCDMode,
  toggleBuzzer,
} from "./api/esp32Api";

const { Title } = Typography;

const MENU_MODES = ["Time", "Temperature", "Alarm", "Countdown", "Weather"];

export default function App() {
  const [status, setStatus] = useState({});
  const [countdownSeconds, setCountdownSeconds] = useState(60);
  const [displayMode, setDisplayMode] = useState(MENU_MODES[0]);

  // Fetch ESP32 status every second
 useEffect(() => {
  const interval = setInterval(async () => {
    try {
      const data = await getStatus();
      setStatus(data);

      // Sync menu from ESP32
      if (data.menuName && data.menuName !== displayMode) {
        setDisplayMode(data.menuName);
      }
    } catch (err) {
      console.error("Failed to fetch status:", err);
    }
  }, 1000);

  return () => clearInterval(interval);
}, [displayMode]);

  const handleUpdateAlarm = () => {
    updateAlarm(status.alarmHour ?? 7, status.alarmMin ?? 0, status.alarmOn ?? false)
      .catch(console.error);
  };

  const handleStartCountdown = () => {
    startCountdown(countdownSeconds).catch(console.error);
  };

  const handleToggleBuzzer = (buzzer) => {
    toggleBuzzer(buzzer).catch(console.error);
  };

  const handleChangeMode = (mode) => {
    setDisplayMode(mode);
    changeLCDMode(mode).catch(console.error);
  };

  return (
    <div className="min-h-screen w-screen bg-gradient-to-r from-blue-50 to-indigo-50 p-6 font-sans flex flex-col items-center">
      <div className="w-full max-w-4xl bg-white rounded-2xl shadow-lg p-6">
        <Title level={2} className="text-blue-700 mb-4 flex items-center gap-3 justify-center">
          <AiOutlineClockCircle className="text-3xl" /> ESP32 Clock Dashboard
        </Title>
        <Divider className="border-blue-200" />

        {/* Mode Selector */}
        <div className="mb-6">
          <ModeSelector
            modes={MENU_MODES}
            currentMode={displayMode}
            setMode={handleChangeMode}
          />
        </div>

        {/* Display Panel */}
        <div className="bg-gray-50 p-6 rounded-xl shadow-inner">
          <DisplayPanel
            displayMode={displayMode}
            status={status}
            setStatus={setStatus}
            updateAlarm={handleUpdateAlarm}
            toggleBuzzer={handleToggleBuzzer}
            countdownSeconds={countdownSeconds}
            setCountdownSeconds={setCountdownSeconds}
            startCountdown={handleStartCountdown}
          />
        </div>
      </div>

      {/* Footer */}
      <footer className="mt-6 text-gray-500 text-sm">
        &copy; Team 6 IOT102_Digital_Clock
      </footer>
    </div>
  );
}
