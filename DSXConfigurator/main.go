package main

import (
	"embed"
	"encoding/json"
	"fmt"
	"io/fs"
	"log"
	"net"
	"net/http"
	"os"
	"os/exec"
	"runtime"
	"sync"
)

//go:embed web/*
var content embed.FS

type ConfigEntry struct {
	Name              string `json:"Name"`
	CustomFormID      string `json:"CustomFormID"`
	Category          string `json:"Category"`
	Description       string `json:"Description"`
	ControllerIndex   int    `json:"ControllerIndex"`
	TriggerSide       int    `json:"TriggerSide"`
	TriggerType       int    `json:"TriggerType"`
	CustomTriggerMode int    `json:"customTriggerMode"`
	TriggerParams     []int  `json:"TriggerParams"`
	RGBUpdate         []int  `json:"RGBUpdate"`
	PlayerLED         []bool `json:"PlayerLED"`
	PlayerLEDNewRev   int    `json:"playerLEDNewRev"`
	MicLEDMode        int    `json:"MicLEDMode"`
	TriggerThreshold  int    `json:"TriggerThreshold"`
}

var configPath = "DSXFallout4Config.json"
var configLock sync.Mutex

// ProtectedNames are the default entries that should not be deleted
var ProtectedNames = map[string]bool{
	"Default Left":                    true,
	"Default Right":                   true,
	"Hand to Hand Block":              true,
	"Hand to Hand Attack":             true,
	"One Hand Sword Block":            true,
	"One Hand Sword Attack":           true,
	"One Hand Dagger Block":           true,
	"One Hand Dagger Attack":          true,
	"One Hand Axe Block":              true,
	"One Hand Axe Attack":             true,
	"One Hand Mace Block":             true,
	"One Hand Mace Attack":            true,
	"Two Hand Sword Block":            true,
	"Two Hand Sword Attack":           true,
	"Two Hand Axe Block":              true,
	"Two Hand Axe Attack":             true,
	"Bow Aim":                         true,
	"Bow Fire":                        true,
	"Staff Aim":                       true,
	"Staff Fire":                      true,
	"Automatic Gun Aim":               true,
	"Automatic Gun Fire":              true,
	"Charging Attack Gun Aim":         true,
	"Charging Attack Gun Fire":        true,
	"Bolt Action Gun Aim":             true,
	"Bolt Action Gun Fire":            true,
	"Hold Input Gun Aim":              true,
	"Hold Input Gun Fire":             true,
	"Regular Gun Aim":                 true,
	"Regular Gun Fire":                true,
	"Charging Reload Gun Aim":         true,
	"Charging Reload Gun Fire":        true,
	"Repeatable Single Fire Gun Aim":  true,
	"Repeatable Single Fire Gun Fire": true,
	"Grenade Aim":                     true,
	"Grenade Throw":                   true,
	"Mine Aim":                        true,
	"Mine Place":                      true,
}

func main() {
	// Try to find the config file
	if _, err := os.Stat(configPath); os.IsNotExist(err) {
		// Try looking in parent directory (if running from a subdir)
		if _, err := os.Stat("../" + configPath); err == nil {
			configPath = "../" + configPath
		} else {
			fmt.Printf("Warning: %s not found in current or parent directory.\n", configPath)
		}
	}

	// Setup file server
	fsys, err := fs.Sub(content, "web")
	if err != nil {
		log.Fatal(err)
	}
	http.Handle("/", http.FileServer(http.FS(fsys)))

	http.HandleFunc("/api/config", handleConfig)
	http.HandleFunc("/api/meta", handleMeta)

	// Try to find an available port
	port := 8080
	var listener net.Listener

	for i := 0; i < 10; i++ {
		listener, err = net.Listen("tcp", fmt.Sprintf(":%d", port))
		if err == nil {
			break
		}
		port++
	}

	if err != nil {
		log.Fatal("Could not find an available port to start the server")
	}

	url := fmt.Sprintf("http://localhost:%d", port)
	fmt.Printf("Starting server at %s\n", url)

	go openBrowser(url)

	log.Fatal(http.Serve(listener, nil))
}

func handleConfig(w http.ResponseWriter, r *http.Request) {
	configLock.Lock()
	defer configLock.Unlock()

	if r.Method == http.MethodGet {
		data, err := os.ReadFile(configPath)
		if err != nil {
			if os.IsNotExist(err) {
				// Return empty array if file doesn't exist
				w.Header().Set("Content-Type", "application/json")
				w.Write([]byte("[]"))
				return
			}
			http.Error(w, "Failed to read config: "+err.Error(), http.StatusInternalServerError)
			return
		}
		w.Header().Set("Content-Type", "application/json")
		w.Write(data)
	} else if r.Method == http.MethodPost {
		var entries []ConfigEntry
		if err := json.NewDecoder(r.Body).Decode(&entries); err != nil {
			http.Error(w, "Invalid JSON", http.StatusBadRequest)
			return
		}

		data, err := json.MarshalIndent(entries, "", "    ")
		if err != nil {
			http.Error(w, "Failed to marshal config", http.StatusInternalServerError)
			return
		}

		if err := os.WriteFile(configPath, data, 0644); err != nil {
			http.Error(w, "Failed to write config: "+err.Error(), http.StatusInternalServerError)
			return
		}

		w.WriteHeader(http.StatusOK)
	} else {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
	}
}

func handleMeta(w http.ResponseWriter, r *http.Request) {
	// Return metadata for dropdowns and tooltips
	meta := map[string]interface{}{
		"ProtectedNames": ProtectedNames,
		"Categories": []string{
			"Default", "HandToHand", "OneHandSword", "OneHandDagger", "OneHandAxe", "OneHandMace",
			"TwoHandSword", "TwoHandAxe", "Bow", "Staff",
			"Gun_Automatic", "Gun_ChargingAttack", "Gun_BoltAction", "Gun_HoldInputToPower",
			"Gun_Regular", "Gun_ChargingReload", "Gun_RepeatableSingleFire",
			"Grenade", "Mine",
		},
		"TriggerTypes": []map[string]interface{}{
			{"Value": 0, "Name": "Normal", "Description": "Default trigger with no resistance"},
			{"Value": 1, "Name": "GameCube", "Description": "Soft initial press with increasing resistance"},
			{"Value": 2, "Name": "VerySoft", "Description": "Very light resistance"},
			{"Value": 3, "Name": "Soft", "Description": "Light resistance"},
			{"Value": 4, "Name": "Hard", "Description": "Moderate resistance"},
			{"Value": 5, "Name": "VeryHard", "Description": "Strong resistance"},
			{"Value": 6, "Name": "Hardest", "Description": "Maximum resistance"},
			{"Value": 7, "Name": "Rigid", "Description": "Completely stiff trigger"},
			{"Value": 8, "Name": "VibrateTrigger", "Description": "Vibration effect", "Params": []string{"Strength (0-8)", "Frequency (0-255)", "Reserved", "Reserved"}},
			{"Value": 9, "Name": "Choppy", "Description": "Intermittent resistance"},
			{"Value": 10, "Name": "Medium", "Description": "Balanced resistance"},
			{"Value": 11, "Name": "VibrateTriggerPulse", "Description": "Pulsing vibration", "Params": []string{"Strength (0-8)", "Frequency (0-255)", "Period (0-255)", "Reserved"}},
			{"Value": 12, "Name": "CustomTriggerValue", "Description": "Custom effect (uses customTriggerMode)", "Params": []string{"Start (0-9)", "Force (0-9)", "Middle (0-9)", "End (0-9)"}},
			{"Value": 13, "Name": "Resistance", "Description": "Adjustable resistance", "Params": []string{"Start (0-9)", "Force (0-9)", "Reserved", "Reserved"}},
			{"Value": 14, "Name": "Bow", "Description": "Simulates drawing a bowstring", "Params": []string{"Start (0-9)", "End (0-9)", "Snap Start (0-9)", "Snap End (0-9)"}},
			{"Value": 15, "Name": "Galloping", "Description": "Rhythmic resistance", "Params": []string{"Start (0-9)", "Middle (0-9)", "End (0-9)", "Frequency (0-9)"}},
			{"Value": 16, "Name": "SemiAutomaticGun", "Description": "Click-like resistance", "Params": []string{"Start (0-9)", "End (0-9)", "Reserved", "Reserved"}},
			{"Value": 17, "Name": "AutomaticGun", "Description": "Rapid resistance pulses", "Params": []string{"Start (0-9)", "End (0-9)", "Frequency (0-9)", "Reserved"}},
			{"Value": 18, "Name": "Machine", "Description": "Complex resistance pattern", "Params": []string{"Start (0-9)", "Middle (0-9)", "End (0-9)", "Frequency (0-9)"}},
		},
		"CustomTriggerModes": []map[string]interface{}{
			{"Value": 0, "Name": "OFF", "Description": "No custom effect"},
			{"Value": 1, "Name": "Rigid", "Description": "Stiff trigger"},
			{"Value": 2, "Name": "RigidA", "Description": "Stiff with variation A"},
			{"Value": 3, "Name": "RigidB", "Description": "Stiff with variation B"},
			{"Value": 4, "Name": "RigidAB", "Description": "Stiff with combined variations"},
			{"Value": 5, "Name": "Pulse", "Description": "Pulsing effect"},
			{"Value": 6, "Name": "PulseA", "Description": "Pulsing with variation A"},
			{"Value": 7, "Name": "PulseB", "Description": "Pulsing with variation B"},
			{"Value": 8, "Name": "PulseAB", "Description": "Pulsing with combined variations"},
		},
		"PlayerLEDModes": []map[string]interface{}{
			{"Value": 0, "Name": "One", "Description": "Single LED lit"},
			{"Value": 1, "Name": "Two", "Description": "Two LEDs lit"},
			{"Value": 2, "Name": "Three", "Description": "Three LEDs lit"},
			{"Value": 3, "Name": "Four", "Description": "Four LEDs lit"},
			{"Value": 4, "Name": "Five", "Description": "All five LEDs lit"},
			{"Value": 5, "Name": "AllOff", "Description": "All LEDs off"},
		},
		"MicLEDModes": []map[string]interface{}{
			{"Value": 0, "Name": "On", "Description": "Mic LED always on"},
			{"Value": 1, "Name": "Pulse", "Description": "Mic LED pulses"},
			{"Value": 2, "Name": "Off", "Description": "Mic LED off"},
		},
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(meta)
}

func openBrowser(url string) {
	var err error
	switch runtime.GOOS {
	case "linux":
		err = exec.Command("xdg-open", url).Start()
	case "windows":
		err = exec.Command("rundll32", "url.dll,FileProtocolHandler", url).Start()
	case "darwin":
		err = exec.Command("open", url).Start()
	default:
		err = fmt.Errorf("unsupported platform")
	}
	if err != nil {
		log.Printf("Failed to open browser: %v", err)
	}
}
