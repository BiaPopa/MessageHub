Popa Bianca

Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

Detalii implementare:

In fisierul corespunzator serverului (server.cpp), se deschid 2 socketi: unul
pentru clientii TCP (listenfd) si unul pentru clientii UDP (udpfd), se dezactiveaza
algoritmul lui Nagle si se apeleaza functia bind pentru a asocia adresa serverului
cu socketii creati. Se apeleaza functia run_chat_multi_server, unde setam socketul
pentru ascultare si adaugam in vectorul de descriptori poll_fds: stdin si cei doi 
socketi. Se apeleaza functia poll pentru a astepta evenimente. 

Daca se primesc date pe socketul TCP, se apeleaza functia accept, acceptandu-se 
astfel noua conexiune. Se verifica daca id-ul clientului se afla deja in vectorul
clientId (ce retine id-urile clientilor deja conectati). Daca da, atunci se inchide
conexiunea si se afiseaza mesajul "Client <ID_CLIENT> already connected.". Daca 
nu se afla, se verifica daca clientul a mai fost conectat. Daca da, atunci se
reconecteaza, se adauga noua conexiune in vectorul de descriptori poll_fds, se 
adauga id-ul clientului in vectorul clientId si se preiau vechile informatii ale 
acestuia din vectorul ce contine clientii deconectati. Se sterge clientul curent
din acest vector si se trimit mesajele primite cat timp a fost deconectat. In final,
se afiseaza mesajul "New client <ID_CLIENT> connected from IP:PORT.". Daca clientul
nu a mai fost conectat, se adauga noua conexiune in vectorul de descriptori poll_fds
si se adauga id-ul clientului in vectorul clientId, afisandu-se mesajul "New client
<ID_CLIENT> connected from IP:PORT.". De asemenea, se adauga in vectorul clients
o noua structura de tip client ce va retine informatii despre abonarile noului
client.

Daca se primesc date pe socketul de la tastatura, se citeste comanda intr-un buffer
si se verifica daca este "exit", caz in care se inchid conexiunile cu toti clientii 
si se iese din program.

Daca se primesc date pe socketul UDP, se parseaza mesajul primit folosind structura
message. Mesajul este trimis apoi catre toti clientii conectati si abonati la 
topicul respectiv si adaugat in vectorul de mesaje al clientilor deconectati si
abonati la topic cu sf-ul 1.

Daca se primesc date pe unul din socketii de client, se verifica daca conexiunea
s-a inchis. Daca da, atunci se elimina informatiile despre client din vectorul
de descriptori poll_fds, vectorul clientId si vectorul clients si se adauga
informatiile acestuia in vectorul de clienti deconectati. Daca conexiunea nu este
inchisa, sunt posibile doua comenzi: "subscribe" si "unsubscribe". Daca comanda
este "subscribe", se verifica daca clientul este deja abonat la topic, caz in care
doar se actualizeaza datele. Daca nu este abonat, se adauga noile date in vectorii
topics, sf si blocked. Daca comanda este "unsubscribe", se actualizeaza datele.

In fisierul corespunzator clientului (subscriber.cpp), se deschide un socket 
(sockfd), se dezactiveaza algoritmul lui Nagle si se realizeaza conectarea la
server. Dupa, se trimite id-ul clientului la server si se apeleaza functia 
run_client, unde adaugam in vectorul de descriptori fds: stdin si socketul
trimis ca parametru. Se apeleaza functia poll pentru a astepta evenimente.

Daca se primesc date de la server, se verifica mai intai daca serverul s-a
inchis, caz in care se iese din program. Daca nu, atunci se afiseaza mesajul
primit. 

Daca se primesc date de la tastatura, sunt posibile trei comenzi: "subscribe",
"unsubscribe" si "exit". Daca se primeste comanda de "subscribe", se adauga
intr-un pachet comanda, topicul si sf-ul, se afiseaza mesajul "Subscribed to 
topic." si se trimite pachetul la server. Daca se primeste comanda de 
"unsubscribe", se adauga intr-un pachet comanda si topicul, se afiseaza mesajul
"Unsubscribed from topic." si se trimite pachetul la server. Daca se primeste 
comanda "exit", se inchide conexiunea cu serverul.

Functiile recv_all si send_all din common.cpp sunt implementate conform celor 
din laborator.

Structura chat_packet din common.h contine campurile: command, topic si sf.
Aceasta este folosita pentru a trimite pachete de la client la server cu
informatii legate de comenzile "subscribe" si "unsubscribe".

Structura message din common.h contine campurile: ip, port, topic, type,
string_payload, int_payload, float_payload, decimals si short_real_payload.
Aceasta este folosita pentru a retine si trimite mai departe mesajele primite
de server de la clientii UDP. Campul string_payload este folosit ca payload
pentru identificatorul STRING, campul int_payload - identificatorul INT,
campul float_payload - identificatorul FLOAT si campul short_real_payload -
identificatorul SHORT_REAL. Campul decimals este folosit pentru a retine numarul
de zecimale necesar pentru afisarea mesajului cu identificatorul FLOAT.

Structura client din common.h contine campurile: id, topics, sf, blocked si
messages. Aceasta este folosita pentru a retine informatii legate de abonarile
clientului. Vectorul topics retine topicurile la care acesta este abonat, vectorul
sf retine sf-urile pentru fiecare topic, vectorul blocked retine topicurile la
care clientul s-a dezabonat, iar vectorul messages retine mesajele primite de client
cat timp acesta a fost deconectat.