<?php header('Access-Control-Allow-Origin: *'); ?>
<!DOCTYPE html>
<html>

<body>

	<p><span class="label label-success">Версия: <span id="firmware_last">2.0 Rev: 01</span> - Дата релиза 15.08.2020.
		</span>
	</p>
	<p>
		Анонс:
		<ul id="ordered">
			<li> 2.0.0:
				<ul id="ordered">
					<li style="color: red">Заливать на чистую FLASH!!!</li>
					<li style="color: red">Радиола переехала на <strong>framework-espidf 4.0.1</strong>.</li>
					<li>Реализована тестовая поддержка модуля BTX01, через консоль UART и TELNET.</li>
					<li>Доступные команды для модуля BTX01 можно посмотреть, набрав в консоли "help".</li>
					<li>Функция регулировки яркости подсветки дисплея реализована в тестовом режиме, без настройки в интерфейсах.</li>
					<li>Поддержка клавиатуры и сопутствующие функции будут реализованы позднее.</li>
				</ul>
			</li>
		</ul>
		</br>
	</p>
</body>

</html>