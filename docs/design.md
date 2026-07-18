## Descriere
(momentan tot codul este in main.cpp, se poate sa il impart in mai multe file uri):

M-am gandit sa fie doua clase principale, Simulator si Strategy
### Simulatorul are in componenta:

**Variabilele membre:**

- timpul - long long, exprimat in ns
- un latency unic (am considerat 3 ms)
- suma de bani cu care incepem
- portofoliu (suma disponibila la un anumit timp si pozitiile pentru fiecare simbol)
- toate OrderBookurile (unordered_map, cheia e simbolul, valoarea este OrderBooku ul specific)
- am folosit un priority queue, ordonarea se face dupa timestamp, daca avem 2 evenimente cu timestampuri egale comparam cu un sequence number unic
- ordere active
- un istoric de filluri (momentan nu fac nimic cu el, ma gandesc ca poate fi util pe viitor)

**Funcțiile membre:**

- constructor
- addOrder, modifyOrder, cancelOrder adauga eventuri in coada
- processAddOrder, processCancelOrder, processFill fac schimbarile efective (dupa ce scoatem din coada)

### Alte clase create:

- Order, Fill
- OrderBookul este impartit (askuri si biduri, fiecare este un map<pret, booklevel>, booklevel este cantitatea + o lista(initial era deque, dar stergerea era O(n)))
- TimerEvent, AddOrderEvent, CancelOrderEvent reprezinta eventuri introduse in coada,
fiecare def propria functie execute (override peste execute() din abstract classul Event) care va aplica schimbarile
- Portfolio, in care retin cati bani mai am si positionStateurile pt fiecare simbol (cantitate si avg entry price, cred ca mai bine memorez total value si dupa fac avg)
- OrderLocation a fost de la o implementare anterioara a BookLevelului, era o lista simplu inlantuita si in OrderLocation tineam minte prev order ca sa pot face stergerile mai rapid, acum folosesc std::list


### Strategia va detine
- fair value logic
- edge calculation
- inventory resistance
- which orders it wants to send
- when it wants to modify or cancel those orders


### Asumptii, logica
- comunicare cu piata implica un anumit delay
- modificarea unui order duce la anularea orderului vechi si adaugarea unuia nou
- exchangeul va executa instant eventurile
- daca avem un order de tip market si nu exista suficienta lichiditate, atunci vom da fill cat de mult posibil si vom anula restul orderului
- daca avem un order de tip limit, fill cat putem si restul devine un nou order
- TimerEvents in cazul in care strategia ar lua decizii pe baza timpului trecut de la o anumita actiune (lasa un order pe piata numai un anumit timp de ex),
daca in acea perioada de timp nu avem niciun market event, se creeaza unul artificial
- simulator e responsabil pentru un singur portofoliu, deci o singura strategie

### Validari
- body-ul constructorului de ordere, verif daca orderul e de tip limit si are pret si daca cantitatea e pozitiva
- pentru OrderBook: sa nu fie crossed, simbolul sa fie corect, bids/asks sa nu aiba cantitati/preturi negative;
din main() putem doar sa adaugam in Simulator eventurile specifice, insa executarea evenimentelor se poate face doar 
dupa ce rulam Simulatorul. Validarea bookului se poate face doar dupa S.run()
- addOrder() sa avem un simbol corect si pretul de la limit sa fie strict pozitiv
- getMarkPrice() sa avem un simbol corect si sa existe cel putin un bid si un ask
- getSpred() - trebuie sa existe cel putin un bid sau un ask
- cazul in care nu pot plasa o comanda pt ca nu am suficient cash nu este inca tratat