<?php header('Access-Control-Allow-Origin: *'); ?>
<!DOCTYPE html>
<html>

<body>

	<p><span class="label label-success">Версия: <span id="firmware_last">2.2 Rev: 3</span> - Дата релиза 20.12.2020.
		</span>
	</p>
	<p>
		Анонс:
		<ul id="ordered">
			<li> 2.2.3:
				<ul id="ordered">
				<li>+ <strong style="color: red">Полностью реструктурирована разметка FLASH-памяти!</strong></li>					
				<li style="color: red"><strong>Заливать на чистую FLASH!!! Прошивки по OTA пока нет!</strong></li>					
				<li>Радиола теперь поддерживает загрузку и выгрузку плейлистов радиостанций в стандартном формате <strong>M3U</strong>.</li>
				<li>Максимальное количество радиостанций в Радиоле ограничено до <strong>128<strong>.</li>
				<li>Индикация выхода нового релиза Радиолы. Цвет логотипа Радиолы в верхнем левом углу на странице
				веб-интерфейса меняется с <strong style="color: green">зелёного</strong> на <strong style="color: red">красный</strong>.</li>
				<li>Реализован сброс Радиолы до заводских настроек через кнопку в веб-интерфейсе на вкладке опций.</li>
				<li>+ Добавлена поддержка двух плейлистов ОБЩИЙ и ИЗБРАННОЕ.</li>
				<li>+ Добавлена выгрузка плейлиста ИЗБРАННОЕ.</li>
				<li>+ Добавлена кнопка добавления станции в списке ИЗБРАННОЕ.</li>
				<li>+ Добавлены подсказки к кнопкм в плейлистах.</li>
				<li>+ Добавлена подсказка в плейлист ОБЩИЙ о том, что радиостанции можно сортировать путём перетаскивания строк.</li>
				<li>+ Исправлен баг: при включении на дисплее отображался пустой индикатор громкости.</li>
				<li>+ Исправлен баг: при проигрывании потока через "быстрое воспроизведение", на дисплее отображалось текущее имя станции из плейлиста.</li>
				<li>+ Переделана полностью система хранения всех данных в Радиоле. Данные теперь хранятся в одном разделе под именем "hardware"</li>
				<li>+ Сделано очень много мелких и крупных изменений во всём коде. Радиола всё больше теряет родственные связи с Ka-Radio....</li>
				</ul>
			</li>
		</ul>
		</br>
	</p>
</body>

</html>