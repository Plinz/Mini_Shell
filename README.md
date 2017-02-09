# Mini_Shell
Projet C Mini_Shell

Problems :

- Jobs in bg don't die when we close the shell

- Command bg fg and stop should take a jobs index and not a pid [working on it]

- Bad comportement (exiting by it-self) :

        $ ./miniShell
        miniShell>xed &
        [0] 15257
        miniShell>xed a &
        [1] 15263
        miniShell>exit
        $
