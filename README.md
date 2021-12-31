# Mini Projeto 1

Ferramenta para modificar permissões de ficheiros.

## Compilar
Utilizar o comando make no terminal para executar o makefile.

## XMOD
O xmod utiliza os seguintes comandos:
    - v: modo verboso. Indica a informação sobre a execução do program
    - R : Recursiva. Utiliza o xmod para informação dentro de um DIR
    - c: Mostra as alterações feitas

Exemplo: ./xmod -R 0777 ms
Troca as permissoes para read, write and execute (0777) do diretório ms de forma recursiva. Se for um ficheiro apenas troca a sua permisão. 


## Funcionalidades Adicionais 
** Tratamento de Sinais ** 
Apos o envio do sinal de interrupção(SIGCONT) , os processos filhos são pausados enquanto esperam pelo sinal do pai. Caso o utilizador decida continuar o processo, o sinal SIGCONT é enviado a todos os filhos. Caso contrario é enviado o SIGKILL.

** Geração de registos de execução ** 
Os registos de execução são guardados num ficheiro especificado pelo utilizador através da variável de ambiente "LOG_FILENAME". Os registos são constituídos pelo instante de tempo imediatamente anterior ao registo, o pid (identificador do processo), o event que é a identificação do tipo de evento que afetou o processo (por exemplo, criação de projeto, terminação de projeto, etc) e informação acerca desse event. Por constrangimento de tempo não implementamos o log dos sinais.



## Autoria 
Ana Maia, up201504108@fe.up.pt
Ricardo Gonçalves Pinto up201806849@fe.up.pt
Pedro Magalhães Moreira Nunes up201905396@fe.up.pt
Maria Benedita Prata Pinto Oliveira e Bacelar up201909937@fe.up.pt


## Contribuição
Ana Maia : 35%
* Os modos e opções
* Troca de permissões
* Geração de registos de execução

Ricardo Gonçalves Pinto : 15%
* Read Me
* RWX nos modos -v e -c

Pedro Magalhães Moreira Nunes : 35%
* Tratamento de Sinais 
* Recursividade do xmod

Maria Benedita Prata Pinto Oliveira e Bacelar : 15%
* RWX nos modos -v e -c




