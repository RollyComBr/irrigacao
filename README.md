# irrigacao
Placa Irrigação de 4 relés

Principais comandos a serem enviados pela serial

Comando para alterar o status do alarme para ativar ou desativar.
{"ent":"st","df":1,"r":1} = Entrada=Status, definição=1(V ou F), relé=1(int de 1 a 4)

Comando para receber os dados sem esperar de minuto a minuto
{"ent":"dd"} = Entrada=Dados

Comando para atualizar a hora
{"ent":"hr","h":17,"m":15} = Entrada=Hora, Hora=17(inteiro 24hs), Minutos = 15(inteiro)

Comando para atualizar a data
{"ent":"dt","d":17,"m":8,"a":2023} = Entrada=data, Dia=17(int), Mês=8(int), Ano=2023(int)

Comando para configurar o horário de ativação e desativação dos dispositivos
{"ent":"al","r":1,"ha":21,"ma":15,"hd":22,"md":15} = Entrada=alarme, rele=1, hora inicio=21, minuto inicio=15, hora fim=22, minuto fim=15}

Comando ativar e desativar os relés manualmente
{"ent":"di","r":2,"s":1} = Entrada=dispositivo, relé=2, Status=1 (V ou F)
