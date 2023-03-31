# *farm*
Progetto Laboratorio di Sistemi Operativi, Universita' di Pisa.

Si chiede di realizzare un programma C, denominato __*farm*__, che implementa lo schema di comunicazione tra
processi e thread mostrato in figura.

<p align="center">
  <img src="utility/img/figura1.png" style="display: block; margin: 0 auto;">
</p>

__*farm*__ è un programma composto da due processi, il primo denominato *MasterWorker* ed il secondo denominato *Collector*.
*MasterWorker*, è un processo multi-threaded composto da un thread *Master* e da ‘*n*’ thread *Worker*
(il numero di thread Worker può essere variato utilizzando l’argomento opzionale ‘-n’ – vedere nel seguito).
Il programma prende come argomenti una lista (eventualmente vuota se viene passata l’opzione ‘-d’) di file
binari contenenti numeri interi lunghi ed un certo numero di argomenti opzionali (le opzioni sono ‘-n’, ‘-q’, ‘-
t’, ‘-d’). Il processo *Collector* viene generato dal processo *MasterWorker*. I due processi comunicano attraverso
una connessione socket AF_LOCAL (AF_UNIX). Viene lasciata allo studente la scelta di quale tra i due
processi fa da processo master per la connessione socket, così come la scelta se usare una sola connessione o
più connessioni, una per ogni *Worker*. Il socket file “__*farm.sck*__”, associato alla connessione AF_LOCAL, deve
essere creato all’interno della directory del progetto e deve essere cancellato alla terminazione del programma.

Il processo *MasterWorker* legge gli argomenti passati alla funzione *main* uno alla volta, verificando che siano
file regolari. Se viene passata l’opzione ‘-d’ che prevede come argomento un nome di directory, viene navigata
la directory passata come argomento e considerando tutti i file e le directory al suo interno.

Il nome del generico file di input (unitamente ad altre eventuali informazioni) viene inviato ad uno dei thread
*Worker* del pool tramite una coda concorrente condivisa (denominata “coda concorrente dei task da elaborare”
in Figura). Il generico thread *Worker* si occupa di leggere dal disco il contenuto dell’intero file il cui nome
ha ricevuto in input, e di effettuare un calcolo sugli elementi letti e quindi di inviare il risultato ottenuto,
unitamente al nome del file, al processo *Collector* tramite la connessione socket precedentemente stabilita.
Il processo *Collector* attende di ricevere tutti i risultati dai *Worker* ed al termine stampa i valori ottenuti sullo
standard output, ordinando la stampa, nel formato seguente:

risultato1 filepath1  
risultato2 filepath2  
risultato3 filepath3

La stampa viene ordinata sulla base del risultato in modo crescente (risultato1<=risultato2<=risultato3, …). Il
calcolo che deve essere effettuato su ogni file è il seguente:

<p align="center">
  <img src="utility/img/figura2.png" style="display: block; margin: 0 auto;">
</p>

dove N è il numero di interi lunghi (long) contenuti nel file, e *result* è l’intero lungo che dovrà essere inviato
al Collector. Ad esempio, supponendo che il file “mydir/*fileX.dat*” passato in input come argomento del main
abbia dimensione 24 bytes, con il seguente contenuto (si ricorda che gli interi lunghi –*long*– sono codificati
con 8 bytes in sistemi Linux a 64bit):

3  
2  
4  

il risultato calcolato dal *Worker* sarà: ![](/utility/img/figura3.png) , quindi il processo Collector stamperà:  
 10 mydir/fileX.dat

Gli argomenti che opzionalmente possono essere passati al processo *MasterWorker* sono i seguenti:
- -n *nthread* specifica il numero di thread *Worker* del processo *MasterWorker* (valore di default 4)
- -q *qlen* specifica la lunghezza della coda concorrente tra il thread *Master* ed i thread *Worker* (valore
di default 8)
- -d *directory-name* specifica una directory in cui sono contenuti file binari ed eventualmente altre
directory contenente file binari; i file binari dovranno essere utilizzati come file di input per il calcolo;
- -t *delay* specifica un tempo in millisecondi che intercorre tra l’invio di due richieste successive ai
thread *Worker* da parte del thread *Master* (valore di default 0)
Il processo *MasterWorker* deve gestire i segnali SIGHUP, SIGINT, SIGQUIT, SIGTERM, SIGUSR1. Alla
ricezione del segnale SIGUSR1 il processo *MasterWorker* notifica il processo Collector di stampare i risultati
ricevuti sino a quel momento (sempre in modo ordinato), mentre alla ricezione degli altri segnali, il processo
deve completare i task eventualmente presenti nella coda dei task da elaborare, non leggendo più eventuali
altri file in input, e quindi terminare dopo aver atteso la terminazione del processo Collector ed effettuato la
cancellazione del socket file. Il processo *Collector* maschera tutti i segnali gestiti dal processo *MasterWorker*.
Il segnale SIGPIPE deve essere gestito opportunamente dai due processi.

__Note__  
La dimensione dei file in input non è limitata ad un valore massimo. Si supponga che la lunghezza del nome
dei file (compreso il pathname) sia al massimo 255 caratteri.
__generafile.c__ genera i file per i tests.  
Lo script Bash (__test.sh__) contenente alcuni semplici test che il programma deve superare.  
I test sono da eseguire su macchina virtuale Xubuntu, [download](http://xubuntu.org/). E' possibile, inoltre, scaricare un disco virtuale compresso dal seguente [link](http://calvados.di.unipi.it/storage/teaching/LinuxVM/xubuntu.vmdk.zip).



