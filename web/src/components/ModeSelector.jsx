import React from "react";

export default function ModeSelector({ modes, currentMode, setMode }) {
  return (
    <div className="flex flex-wrap gap-3 mb-6 justify-center">
      {modes.map((mode) => (
        <button
          key={mode}
          onClick={() => setMode(mode)}
          className={`px-5 py-2 rounded-lg font-medium transition-colors duration-200
            ${currentMode === mode ? "bg-blue-600 text-white shadow-lg" : "bg-gray-200 hover:bg-gray-300 text-gray-800"}`}
        >
          {mode}
        </button>
      ))}
    </div>
  );
}
