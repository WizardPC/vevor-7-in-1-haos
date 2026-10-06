#!/usr/bin/env python3
"""Retrouve un ESP32 ESPHome sur le reseau en scannant le port 6053 (API).

Utile car le mDNS ne traverse pas la passerelle entre 192.168.2.0/24 et
172.16.0.0/24 : si la carte change d'adresse apres un redemarrage, la seule
faccon de la retrouver est de sonder le sous-reseau.

Usage : python3 find_esphome.py 172.16.0.0/24 [port]
"""
import concurrent.futures as cf
import ipaddress
import socket
import sys


def probe(ip: str, port: int, timeout: float = 0.6) -> str | None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(timeout)
        try:
            s.connect((ip, port))
        except OSError:
            return None
        return ip


def main() -> int:
    net = ipaddress.ip_network(sys.argv[1] if len(sys.argv) > 1 else "172.16.0.0/24")
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 6053
    hosts = [str(h) for h in net.hosts()]
    print(f"sondage de {len(hosts)} hotes sur {net} port {port}...", flush=True)
    found: list[str] = []
    with cf.ThreadPoolExecutor(max_workers=128) as pool:
        for res in pool.map(lambda ip: probe(ip, port), hosts):
            if res:
                found.append(res)
                print(f"  TROUVE : {res}:{port}", flush=True)
    print(f"resultat : {found if found else 'aucun hote avec ce port ouvert'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
