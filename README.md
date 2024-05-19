# Sistema de alarme para 4 relés (main_alarme.ino)

### Pinos utilizados e livres no arduino

**Bluetooth:** 
>TXD=6, RXD=7

**74HC595:** 
>SH_CP=4, ST_CP=3, DS=2

**Analógicos:** 
>SDA = A4, SCL = A5

**Pinos de Comunicação Livres** 
>RX, TX

**Pinos Digitais Livres:** 
>5, 8, 9, 10, 11, 12, 13

**Pinos Analogicos Livres** 
>A0, A1, A2, A3, A6, A7

___________________________________________________________

### Pinos usados e livres no 74HC595

**Pinos Relé**
>0, 1, 2, 3

**Pinos Livre**
>4, 5, 6, 7

___________________________________________________________

# Placa Irrigação de 4 relés

## Principais comandos a serem enviados pela serial

### Comando para alterar o status do alarme para ativar ou desativar:

**Entrada=Status, alarme=1(int de 1 a 4), status=1(V ou F)**

>{"ent":"st","a":1,"s":1}

### Comando para receber os dados sem esperar de minuto a minuto:

**Entrada=Dados**

>{"ent":"dd"}

### Comando para atualizar a hora:

**Entrada=Hora, GmtZone=-3, Hora=1695105003(Epoch & Unix Timestamp)**

>{"ent":"hr","gmt":"-3","h":1716050337}

### Comando para atualizar a data:

**Entrada=data, Dia=17(int), Mês=8(int), Ano=2023(int)**

>{"ent":"dt","h":3,"m":31,"s":0,"d":17,"M":8,"a":2023}

### Comando para configurar o horário de ativação e desativação dos dispositivos:

**Entrada=alarme, rele=1, hora inicio=21, minuto inicio=15, hora fim=22, minuto fim=15**

>{"ent":"al","r":1,"ha":21,"ma":15,"hd":22,"md":15}

### Comando ativar e desativar os relés manualmente:

**Entrada=dispositivo, relé=2, Status=1 (V ou F)**

>{"ent":"di","r":2,"s":1}

### Comando resetar todas configurações:

**Entrada=Reset**

>{"ent":"rs"}

### Comando editar dias da semana que pode alarmar:

**Entrada=semana, alarme=1, dias=DSTQQSS (1 ou 0)**

>{"ent":"wk","a":1,"d":"1000001"}
