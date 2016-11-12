# Invader Reversed

# Introduction

Invader Reversed is a retro roguelike game modified from original 'Space Invader' game by Andy Thomason.

It contains an invader (main character), enemy ships which keep refreshing from top to bottom and a boss. Player needs to control the invader to kill enemy ships by firing missiles. Any collision to enemies or hitting by bombs will be considered as a game over.
Player should kill the boss to win the game.

# Control

The main character can be moved in four directions, using 4 arrow keys. This is changed from the original left/right movement. Firing missiles are triggered by space button, and you can press F1 to restart the game.

# Essential Updates / Modification
# Roguelike Features
This is part of the early amendments in the codes. It includes enlarging the output screen, change of speed (both enemy ships and the player) and reducing player's life to 1. The principle was to increase difficulties and flexibility for gameplay.

# Enemies
There are two types of enemies: enemy ships and boss ships.

The enemy ships are modified from the original enemy invaders. Firstly, the developer changed the movement from horizontal to vertical. It is intended to generate the feeling that enemies keep refreshing and rush to the player.

A collider is created for enemy ships so that collision between the player and enemy will cause a game over. It increases the difficulty because the player has to dodge both bombs and enemies.

The boss moves horizontally on the top of the screen and it shoots similar to enemy ships. However, it has 5 lives so that the player has to hit it 5 times to win the game. Some new sprites are created for the boss and boss's bombs. New functions are created such as hitting the boss and monitoring boss's lives.

# Time Simulation
Time simulation is essential for the game because refreshing invaders and generating boss are depending on time (actually is game frames).

It is simulated by recording frames of the game, using get_frame_number(). The texts on the top left are also modified to score and time.

# Restart Game
This feature was designed in order to redraw everything and restart the game. However, the process was challenging because the program is still running and the frame number keeps increasing. It results in refreshing enemies and bosses after F1 is pressed. Moreover, some enemy sprites were not disabled so the images of enemies were still appeared on the screen.

The problem of refreshing enemies is solved by adding a frame control variable. The variable, tempframe, records the exact frame number while game over is true. After pressing F1, the new game's frame number equals to actual frame number minus tempframe; in other words, it resets the time and frame number to zero. By using frame control, the enemies can be refreshed correctly in every game.

# Animation
The method of adding animation was contributed by Msc student Abdullah Bin Abdullah.

The animation of the sprite is achieved by reading different part of a single GIF image. The GIF contains each frame of the animation, and uv settings are changed to read those frames in a particular order (in this case, from top left to bottom right).

Firstly, the numbers of rows and columns in the GIF are imported into void init(...). They are used to confirm four vertices of the UV corner. While the animation starts, the frame number of animation keeps going up and the uv vertices will be recalculated to point to the right part of the image.

The main character has 4 animation triggered by four arrow keys. It is achieved by reading certain range of frames in the image. In this case, 4 rows represent four different animations.

A speed control variable was also added to slow the animation. Since the animation function is called for every game frame, the speed control variable can be used to skip certain game frames so the uv pointers will not move.

# Bug Fixing
# Refreshing enemies
Many efforts are taken to refresh enemies correctly. The original invaders are drawn by rows and columns, which are not suitable for the new mechanics. The original loops were broken down and a variable 'refresher' are called to draw new enemy sprites. There are problems that new enemy sprites took over other sprites such as missiles and bombs. It is solved by modifying sprite definitions.

# Enemy shooting
It is monitored that enemies outside the border shoot as well. Because enemy bombs will only be disabled when they touch the border, so enabled bombs outside the screen will not be disabled. It eventually causes enemies in the screen refusing to shoot.

The bug is fixed via different methods. First, pre-drawn enemies should stay away from the player sprite in index X because the bombs are triggered by the relationship of index X between enemy and player. Moreover, a function called invaders_collide() was added to wipe out enemy sprites when they touch the bottom. It controls every enabled sprites inside the screen to prevent inappropriate activation of enemies.

# Game Display and Overlapping
There were many bugs in the game display, especially overlapping images and wrong collision. Most of these bugs were fixed during adding new functions. During final adjustments, it is noticed that some bugs were caused by enlarging the screen in the beginning. Some of the images were stretched so it looked rough and collide incorrectly.

Extra care was taken while importing new artwork to the game in order to adjust the collider and achieve better image quality.

# Credits
Orinial game: Space Invader, developed by Andy Thomason

Modified by Zhouyu Wu

Visual artwork by Zhouyu Wu

Sound sources: http://soundbible.com/

Special thanks to

Andy Thomason: setting time simulation, getting game frames

Abdullah Bin Abdullah: creating animation, changing uv settings
