<?php header('Access-Control-Allow-Origin: *'); ?>
<!DOCTYPE html>
<html>

<body>

	<p><span class="label label-success">Версия: <span id="firmware_last">2.1 Rev: 0</span> - Дата релиза 10.11.2020.
		</span>
	</p>
	<p>
		Анонс:
		<ul id="ordered">
			<li> 2.1.0:
				<ul id="ordered">
					<li style="color: red">**Проект претерпел существенные изменения**</li>
					<li style="color: red"><strong>Заливать на чистую FLASH!!! Прошивки по OTA пока нет!</strong></li>
					<li>У Радиолы теперь две прошивки.</br><strong>**Релизная**</strong> - для постоянного пользования.</br>
					<strong>**Отладочная**</strong> - для анализа поведения, отадки и поиска возможных ошибок.</li>
					<li>+ Вся забота об отслеживании изменений в файлах, необходимых для работы веб-интерфейса, теперь лежит на PlatformIO. При любом изменени</br>
					в этих файлах, автоматически создаются заголовочные файлы с массивами данных в формате языка С.</li>
					<li>+ Проект Радиола можно запускать как на Linux, так и на Windows. (<strong>**на Windows пока не тестировалось!**</strong>)</li>
				</ul>
			</li>
		</ul>
		</br>
	</p>
</body>

</html>