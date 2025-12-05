## Serveur du cadenas

### Prérequis

- Node.js installé (inclut `npm`).

### Installation

```bash
  npm install
```

### Démarrer le serveur

```bash
  npm start
```

Le serveur démarre sur l'hôte (ajouter votre ip) et le port configurés en haut de `server.js` :

```js
const HOST = "0.0.0.0";
const PORT = 3000;
```

### Connexion de l'objet (ESP32)

Dans `cadenas/main/main.c`, configure l'adresse IP du PC qui lance le serveur :

```c
#define SERVER_IP   "0.0.0.0"  // IP de ton PC
#define SERVER_PORT 3000
```

Recompile ensuite le projet ESP-IDF et flashe l'ESP32.

