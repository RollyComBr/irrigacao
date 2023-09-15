# irrigacao
Placa Irrigação de 4 relés

Principais comandos a serem enviados pela serial

________________________________________________________________________________________

Comando para alterar o status do alarme para ativar ou desativar.

Entrada=Status, definição=1(V ou F), relé=1(int de 1 a 4)

{"ent":"st","df":1,"r":1}

________________________________________________________________________________________

Comando para receber os dados sem esperar de minuto a minuto

Entrada=Dados

{"ent":"dd"}

________________________________________________________________________________________

Comando para atualizar a hora

Entrada=Hora, Hora=17(inteiro 24hs), Minutos = 15(inteiro)

{"ent":"hr","h":17,"m":15}

________________________________________________________________________________________

Comando para atualizar a data

Entrada=data, Dia=17(int), Mês=8(int), Ano=2023(int)

{"ent":"dt","d":17,"m":8,"a":2023}

________________________________________________________________________________________

Comando para configurar o horário de ativação e desativação dos dispositivos

Entrada=alarme, rele=1, hora inicio=21, minuto inicio=15, hora fim=22, minuto fim=15

{"ent":"al","r":1,"ha":21,"ma":15,"hd":22,"md":15}

________________________________________________________________________________________

Comando ativar e desativar os relés manualmente

Entrada=dispositivo, relé=2, Status=1 (V ou F)

{"ent":"di","r":2,"s":1}
