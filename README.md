#  Tema 1 - Router
## Student: Cosmin-Razvan VANCEA - 323CA
----------------------------------------

### Module:
* Tema a fost impartita pe mai multe module:
  1. ARPTable:
    * Contine raspunsurile la ARP Requests
    * Se memoreaza o asociere intre IP si MAC
    * Implementata cu `unordered_map`, deci insert si lookup -> O(1)
  2. IPPacketQueue:
    * Contine pachetele care nu pot fi rutate mai departe deoarece nu se
    cunoaste adresa MAC a urmatorului hop
    * Se memoreaza o asociere intre IP-ul urmatorului hop si o coada de pachete
    care urmeaza sa fie trimise cand se va primi ARP Reply
    * Implementata cu `unordered_map`, deci insert si lookup -> O(1)
  3. RoutingTable:
    * Contine intrarile tabelei de rutare
    * Este implementata prin trie, deci insert si lookup -> O(1)
  4. Net:
    * Contine abstractizari ale urmatoarelor structuri:
      * adresa IP
      * adresa MAC
      * interfata
    * Usureaza aplicarea de operatii pe acestea (comparare, convertire etc.)
  5. Router:
    * Modulul principal care se ocupa de primirea, interpretarea si transmiterea
    mai departe a pachetelor.
    * Implementat dupa logica din PDF, cu cateva adaugari explicate mai jos


### Probleme intalnite:
* In principal, problemele intalnite au fost la implementarea ICMP: nicaieri in
  tema/laborator nu este specificat ca un pachet ICMP poate contine si un payload
  si nici faptul ca ICMP checksum-ul se aplica pe **payload** SI pe header.
* De exemplu: cand utilitarul `ping` trimite un ECHO, adauga dupa header-ul ICMP
  un payload de X bytes, iar cand primeste ICMP ECHOREPLY, `ping` se asteapta ca
  payload-ul sa fie returnat in pachetul de reply.
  Daca payload-ul nu se gaseste in reply sau checksum-ul nu este corect (nu se
  face si pe payload), atunci `ping` nu ia in considerare ECHOREPLY.
* Ce facea problema mai greu de depistat era faptul ca desi `ping` dadea fail
  si Wireshark imi spunea ca nu se gaseste reply pentru ECHO, pe checker primeam
  punctaj pe testul de ping.
  Abia mai tarziu mi-am dat seama ca asta se intampla deoarece checkerul verifica
  daca routerul trimite inapoi un ECHOREPLY, nu verifica si daca pachetul contine
  payload-ul initial. (cred ca ping-urile date de checker oricum nu contin niciun
  payload, deci nu are ce sa verifice in ECHOREPLY :-?)
* Am mai aflat ca la trimiterea unui pachet de tip ICMP error (expired TTL etc.),
  in payload-ul pachetului ICMP se pune header-ul IP + primii 8 bytes de dupa
  header-ul IP ai pachetului unde a fost depistata eroarea.

### BONUS:
* Am implementat algoritmul de incremental checksum update pentru actualizarea TTL.
  (vezi functia `IncrementalChecksum` din `Router.cpp`)
