const express = require("express");
const bodyParser = require("body-parser");
const fs = require("fs");
const app = express();

// Configuration du serveur à modifier
const HOST = "192.168.2.10";
const PORT = 3000;

const DATA_FILE = "data.json";

app.use(bodyParser.json());
app.use(express.static("public"));

let record = {
    name: "Personne",
    time_ms: null
};

let attempts = [];

function loadData() {
    try {
        if (fs.existsSync(DATA_FILE)) {
            const raw = fs.readFileSync(DATA_FILE, "utf8");
            const parsed = JSON.parse(raw);
            if (parsed.record) {
                record = parsed.record;
            }
            if (parsed.attempts) {
                attempts = parsed.attempts.slice(-10);
            }
        }
    } catch (err) {
        console.error("Failed to load data:", err);
    }
}

function saveData() {
    const data = { record, attempts };
    fs.writeFile(DATA_FILE, JSON.stringify(data, null, 2), (err) => {
        if (err) {
            console.error("Failed to save data:", err);
        }
    });
}

loadData();

app.post("/submit", (req, res) => {
    const { event, time_ms } = req.body;

    if (event !== "victory") {
        return res.json({ status: "ignored" });
    }

    attempts.push(time_ms);
    if (attempts.length > 10) {
        attempts = attempts.slice(-10);
    }

    let beaten = false;
    if (record.time_ms === null || time_ms < record.time_ms) {
        beaten = true;
        record.time_ms = time_ms;
    }

    saveData();

    res.json({
        beaten,
        record
    });
});

app.post("/setname", (req, res) => {
    const { name, time_ms } = req.body;

    record.name = name;
    record.time_ms = time_ms;

    saveData();

    res.json({ status: "ok", record });
});

app.get("/record", (req, res) => {
    res.json({ record, attempts });
});

app.post("/reset", (req, res) => {
    record = {
        name: "Personne",
        time_ms: null
    };
    attempts = [];

    saveData();

    res.json({ status: "ok" });
});

app.listen(PORT, HOST, () => {
    console.log(`Server running on http://${HOST}:${PORT}`);
});
