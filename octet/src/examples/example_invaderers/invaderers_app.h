////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// invaderer example: simple game with sprites and sounds
//
// Level: 1
//
// Demonstrates:
//   Basic framework app
//   Shaders
//   Basic Matrices
//   Simple game mechanics
//   Texture loaded from GIF file
//   Audio
//

namespace octet {
	class sprite {
		// where is our sprite (overkill for a 2D game!)
		mat4t modelToWorld;

		// half the width of the sprite
		float halfWidth;

		// half the height of the sprite
		float halfHeight;

		// what texture is on our sprite
		int texture;

		// true if this sprite is enabled.
		bool enabled;

		int frame_number, total_frame_x, total_frame_y;
		float texture_width, texture_height, frame_width, frame_height;

		float frameleft, frameright, frameup, framebottom;
		float uvs[8];
	public:
		sprite() {
			texture = 0;
			enabled = true;
		}
		
		void calculateframe() {
			frameleft = (frame_number % total_frame_x) * frame_width;
			frameright = frameleft + frame_width;
			frameup = texture_height - ((frame_number / total_frame_x) * frame_height);
			framebottom = frameup - frame_height;
			frameleft = frameleft / texture_width;
			frameright = frameright / texture_width;
			frameup = frameup / texture_height;
			framebottom = framebottom / texture_height;
		}
		
		void init(int _texture, float x, float y, float w, float h, int numFrameX, int numFrameY) {
			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			halfWidth = w * 1.0f;
			halfHeight = h *1.0f;
			texture = _texture;
			enabled = true;
			frame_number = 0;
			total_frame_x = numFrameX;
			total_frame_y = numFrameY;
			texture_width = w;
			texture_height = h;
			frame_width = texture_width / total_frame_x;
			frame_height = texture_height / total_frame_y;
		}

		void render(texture_shader &shader, mat4t &cameraToWorld) {
			// invisible sprite... used for gameplay.
			if (!texture) return;

			// build a projection matrix: model -> world -> camera -> projection
			// the projection space is the cube -1 <= x/w, y/w, z/w <= 1
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld * 2, cameraToWorld * 2);

			// set up opengl to draw textured triangles using sampler 0 (GL_TEXTURE0)
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);

			// use "old skool" rendering
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			shader.render(modelToProjection, 0);

			// this is an array of the positions of the corners of the sprite in 3D
			// a straight "float" here means this array is being generated here at runtime.
			float vertices[] = {
				-halfWidth, -halfHeight, 0,
				halfWidth, -halfHeight, 0,
				halfWidth,  halfHeight, 0,
				-halfWidth,  halfHeight, 0,
			};

			// attribute_pos (=0) is position of each corner
			// each corner has 3 floats (x, y, z)
			// there is no gap between the 3 floats and hence the stride is 3*sizeof(float)
			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)vertices);
			glEnableVertexAttribArray(attribute_pos);

			// calculate frame
			calculateframe();

			// this is an array of the positions of the corners of the texture in 2D
			uvs[0] = frameleft; uvs[1] = framebottom;
			uvs[2] = frameright; uvs[3] = framebottom;
			uvs[4] = frameright; uvs[5] = frameup;
			uvs[6] = frameleft; uvs[7] = frameup;

			// attribute_uv is position in the texture of each corner
			// each corner (vertex) has 2 floats (x, y)
			// there is no gap between the 2 floats and hence the stride is 2*sizeof(float)
			glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)uvs);
			glEnableVertexAttribArray(attribute_uv);

			// finally, draw the sprite (4 vertices)
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		// move the object
		void translate(float x, float y) {
			modelToWorld.translate(x, y, 0);
		}

		// position the object relative to another.
		void set_relative(sprite &rhs, float x, float y) {
			modelToWorld = rhs.modelToWorld;
			modelToWorld.translate(x, y, 0);
		}

		// return true if this sprite collides with another.
		// note the "const"s which say we do not modify either sprite
		bool collides_with(const sprite &rhs) const {
			float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];
			float dy = rhs.modelToWorld[3][1] - modelToWorld[3][1];

			// both distances have to be under the sum of the halfwidths
			// for a collision
			return
				(fabsf(dx) < halfWidth + rhs.halfWidth) &&
				(fabsf(dy) < halfHeight + rhs.halfHeight)
				;
		}

		bool is_above(const sprite &rhs, float margin) const {
			float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];
			float dy = rhs.modelToWorld[3][1] - modelToWorld[3][1];
			return
				(fabsf(dx) < halfWidth + margin) && (fabsf(dy) > 0)
				;
		}

		bool &is_enabled() {
			return enabled;
		}
	};

  class invaderers_app : public octet::app {
    // Matrix to transform points in our camera space to the world.
    // This lets us move our camera
    mat4t cameraToWorld;

    // shader to draw a textured triangle
    texture_shader texture_shader_;
	boss_shader boss_shader_;

    enum {
      num_sound_sources = 8,
      num_first = 4,
      num_second = 3,
	  num_extra = 4,
      num_missiles = 5,
      num_bombs = 6,
      num_borders = 4,
      num_invaderers = num_first * num_second,

      // sprite definitions
      ship_sprite = 0,
      boss_sprite,
      first_invaderer_sprite,
      last_invaderer_sprite = first_invaderer_sprite + num_invaderers - 1,

      first_missile_sprite,
      last_missile_sprite = first_missile_sprite + num_missiles - 1,

      first_bomb_sprite,
      last_bomb_sprite = first_bomb_sprite + num_bombs - 1,

	  first_bossbomb_sprite,
	  last_bossbomb_sprite = first_bossbomb_sprite + num_bombs -1,

      first_border_sprite,
      last_border_sprite = first_border_sprite + num_borders - 1,

      num_sprites,

	  game_over_sprite,

    };

    // timers for missiles and bombs
    int missiles_disabled;
    int bombs_disabled;

    // accounting for bad guys
	int boss_lives;
    int num_lives;

    // game state
    bool game_over;
    int score;
	int timer=0;
	int refresher = 0;
	bool boss_exist = false;

    // speed of enemy
    float invader_velocity;
	float boss_velocity;

    // sounds
    ALuint whoosh;
    ALuint bang;
    unsigned cur_source;
    ALuint sources[num_sound_sources];

    // big array of sprites
    sprite sprites[num_sprites];

    // random number generator
    class random randomizer;

    // a texture for our text
    GLuint font_texture;

    // information for our text
    bitmap_font font;

    ALuint get_sound_source() { return sources[cur_source++ % num_sound_sources]; }

    // called when we hit an enemy
    void on_hit_invaderer() {
      ALuint source = get_sound_source();
      alSourcei(source, AL_BUFFER, bang);
      alSourcePlay(source);
      score++;
    }

    // called when we are hit
    void on_hit_ship() {
      ALuint source = get_sound_source();
      alSourcei(source, AL_BUFFER, bang);
      alSourcePlay(source);

      if (--num_lives == 0) {
        game_over = true;
        sprites[game_over_sprite].translate(-20, 0);
      }
    }

    // use the keyboard to move the ship
    void move_ship() {
      const float ship_speed = 0.2f;
      // left and right arrows
      if (is_key_down(key_left)) {
        sprites[ship_sprite].translate(-ship_speed, 0);
        if (sprites[ship_sprite].collides_with(sprites[first_border_sprite+2])) {
          sprites[ship_sprite].translate(+ship_speed, 0);
        }
      } else if (is_key_down(key_right)) {
        sprites[ship_sprite].translate(+ship_speed, 0);
        if (sprites[ship_sprite].collides_with(sprites[first_border_sprite+3])) {
          sprites[ship_sprite].translate(-ship_speed, 0);
        }
      }
	  // up and down arrows
	  if (is_key_down(key_up)) {
		  sprites[ship_sprite].translate(0, +ship_speed);
		  if (sprites[ship_sprite].collides_with(sprites[first_border_sprite + 1])) {
			  sprites[ship_sprite].translate(0, -ship_speed);
		  }
	  }
	  else if (is_key_down(key_down)) {
		  sprites[ship_sprite].translate(0, -ship_speed);
		  if (sprites[ship_sprite].collides_with(sprites[first_border_sprite + 0])) {
			  sprites[ship_sprite].translate(0, +ship_speed);
		  }
	  }
    }

    // fire button (space)
    void fire_missiles() {
      if (missiles_disabled) {
        --missiles_disabled;
      } else if (is_key_going_down(' ')) {
        // find a missile
        for (int i = 0; i != num_missiles; ++i) {
          if (!sprites[first_missile_sprite+i].is_enabled()) {
            sprites[first_missile_sprite+i].set_relative(sprites[ship_sprite], 0, 0.5f);
            sprites[first_missile_sprite+i].is_enabled() = true;
            missiles_disabled = 5;
            ALuint source = get_sound_source();
            alSourcei(source, AL_BUFFER, whoosh);
            alSourcePlay(source);
            break;
          }
        }
      }
    }

    // pick and invader and fire a bomb
    void fire_bombs() {
      if (bombs_disabled) {
        --bombs_disabled;
      } else {
        // find an invaderer
        sprite &ship = sprites[ship_sprite];
		int j = randomizer.get(0, num_invaderers);
          sprite &invaderer = sprites[first_invaderer_sprite+j];
          if (invaderer.is_enabled() && invaderer.is_above(ship, 0.3f)) {
            // find a bomb
            for (int i = 0; i != num_bombs; ++i) {
              if (!sprites[first_bomb_sprite+i].is_enabled()) {
                sprites[first_bomb_sprite+i].set_relative(invaderer, 0, -0.25f);
                sprites[first_bomb_sprite+i].is_enabled() = true;
                bombs_disabled = 30;
                ALuint source = get_sound_source();
                alSourcei(source, AL_BUFFER, whoosh);
                alSourcePlay(source);
                return;
              }
            }
        }
      }
    }

	// pick the boss to fire bombs
	void fire_boss() {
		if (bombs_disabled) {
			--bombs_disabled;
		}
		else {
			sprite &ship = sprites[ship_sprite];
				sprite &invaderer = sprites[boss_sprite];
				if (invaderer.is_enabled() && invaderer.is_above(ship, 0.5f)) {
					// find a bomb
					for (int i = 0; i != num_bombs; ++i) {
						if (!sprites[first_bossbomb_sprite + i].is_enabled()) {
							sprites[first_bossbomb_sprite + i].set_relative(invaderer, 0, -0.25f);
							sprites[first_bossbomb_sprite + i].is_enabled() = true;
							bombs_disabled = 30;
							ALuint source = get_sound_source();
							alSourcei(source, AL_BUFFER, whoosh);
							alSourcePlay(source);
							return;
						}
					}
					return;
				}
			}
	}

    // animate the missiles
	// check if missles hit invaders or boss
    void move_missiles() {
      const float missile_speed = 0.5f;
      for (int i = 0; i != num_missiles; ++i) {
        sprite &missile = sprites[first_missile_sprite+i];
        if (missile.is_enabled()) {
          missile.translate(0, missile_speed);
          for (int j = 0; j != num_invaderers; ++j) {
            sprite &invaderer = sprites[first_invaderer_sprite+j];
            if (invaderer.is_enabled() && missile.collides_with(invaderer)) {
              invaderer.is_enabled() = false;
              invaderer.translate(20, 0);
              missile.is_enabled() = false;
              missile.translate(20, 0);
              on_hit_invaderer();
              goto next_missile;
            }
          }

		  sprite &boss = sprites[boss_sprite];
		  if (boss.is_enabled() && missile.collides_with(boss)) {
			  --boss_lives;
			  missile.is_enabled() = false;
			  missile.translate(20, 0);
			  on_hit_invaderer();
			  goto next_missile;
		  }

          if (missile.collides_with(sprites[first_border_sprite+1])) {
            missile.is_enabled() = false;
            missile.translate(20, 0);
          }
        }
      next_missile:;
      }
    }

    // animate the bombs
    void move_bombs() {
      const float bomb_speed = 0.5f;
      for (int i = 0; i != num_bombs; ++i) {
        sprite &bomb = sprites[first_bomb_sprite+i];
        if (bomb.is_enabled()) {
          bomb.translate(0, -bomb_speed);
          if (bomb.collides_with(sprites[ship_sprite])) {
            bomb.is_enabled() = false;
            bomb.translate(20, 0);
            bombs_disabled = 50;
            on_hit_ship();
            goto next_bomb;
          }
          if (bomb.collides_with(sprites[first_border_sprite+0])) {
            bomb.is_enabled() = false;
            bomb.translate(20, 0);
          }
        }
      next_bomb:;
      }
    }

	// animate the boss bombs
	void move_bossbombs() {
		const float bomb_speed = 0.5f;
		for (int i = 0; i != num_bombs; ++i) {
			sprite &bomb = sprites[first_bossbomb_sprite + i];
			if (bomb.is_enabled()) {
				bomb.translate(0, -bomb_speed);
				if (bomb.collides_with(sprites[ship_sprite])) {
					bomb.is_enabled() = false;
					bomb.translate(20, 0);
					bombs_disabled = 50;
					on_hit_ship();
					goto next_bomb;
				}
				if (bomb.collides_with(sprites[first_border_sprite + 0])) {
					bomb.is_enabled() = false;
					bomb.translate(20, 0);
				}
			}
		next_bomb:;
		}
	}

    // move the array of enemies
    void move_invaders(float dx, float dy) {
      for (int j = 0; j != num_invaderers; ++j) {
        sprite &invaderer = sprites[first_invaderer_sprite+j];
        if (invaderer.is_enabled()) {
          invaderer.translate(dx, dy);
        }
      }
    }

	// move the array of boss
	void move_boss(float dx, float dy) {
		sprite &invaderer = sprites[boss_sprite];
			if (invaderer.is_enabled()) {
				invaderer.translate(dx, dy);
			}
	}

	// check collision with invaders
	void ship_collide() {
		for (int i = 0; i != num_invaderers; ++i) {
			sprite &invaderer = sprites[first_invaderer_sprite + i];
			if (invaderer.is_enabled() && invaderer.collides_with(sprites[ship_sprite])) {
				on_hit_ship();
			}
		}
	}

	// check collision between invaders
	void collider() {
		for (int i = 0; i != num_invaderers; ++i) {
			sprite &invaderer = sprites[first_invaderer_sprite + i];
			if (invaderer.is_enabled() && invaderer.collides_with(sprites[first_invaderer_sprite + refresher]) && i != refresher) {
				invaderer.is_enabled() = false;
				invaderer.translate(20, 0);
			}
		}
	}

	// check if boss hit the side & move boss
	void boss_collide() {
		if (sprites[boss_sprite].collides_with(sprites[first_border_sprite + 2])) {
			boss_velocity = -boss_velocity;
			move_boss(boss_velocity, 0);
		}
		if (sprites[boss_sprite].collides_with(sprites[first_border_sprite + 3])) {
			boss_velocity = -boss_velocity;
			move_boss(boss_velocity, 0);
		}
	}

    // check if any invaders hit the bottom
	void invaders_collide() {
		for (int i = 0; i != num_invaderers; ++i) {
			sprite &invaderer = sprites[first_invaderer_sprite + i];
			if (invaderer.is_enabled() && invaderer.collides_with(sprites[first_border_sprite + 0])) {
				invaderer.is_enabled() = false;
				invaderer.translate(20, 0);
			}
		}
	}
 
    void draw_text(texture_shader &shader, float x, float y, float scale, const char *text) {
      mat4t modelToWorld;
      modelToWorld.loadIdentity();
      modelToWorld.translate(x, y, 0);
      modelToWorld.scale(scale, scale, 1);
      mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

      /*mat4t tmp;
      glLoadIdentity();
      glTranslatef(x, y, 0);
      glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);
      glScalef(scale, scale, 1);
      glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);*/

      enum { max_quads = 32 };
      bitmap_font::vertex vertices[max_quads*4];
      uint32_t indices[max_quads*6];
      aabb bb(vec3(0, 0, 0), vec3(256, 256, 0));

      unsigned num_quads = font.build_mesh(bb, vertices, indices, max_quads, text, 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, font_texture);

      shader.render(modelToProjection, 0);

      glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x );
      glEnableVertexAttribArray(attribute_pos);
      glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u );
      glEnableVertexAttribArray(attribute_uv);

      glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
    }

  public:

    // this is called when we construct the class
    invaderers_app(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt") {
    }

    // this is called once OpenGL is initialized
    void app_init() {
      // set up the shader
      texture_shader_.init();

      // set up the matrices with a camera 5 units from the origin
      cameraToWorld.loadIdentity();
      cameraToWorld.translate(0, 0, 3);

      font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");

      GLuint ship = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/ship_new.gif");
      sprites[ship_sprite].init(ship, 0, -2.75f, 0.5f, 0.5f, 2, 2);

      GLuint GameOver = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/GameOver.gif");
      sprites[game_over_sprite].init(GameOver, 20, 0, 3, 1.5f,1, 1);
	  
      // set the border to white for clarity
      GLuint white = resource_dict::get_texture_handle(GL_RGB, "#ffffff");
      sprites[first_border_sprite+0].init(white, 0, -6, 6, 0.2f, 1, 1);
      sprites[first_border_sprite+1].init(white, 0, 6, 6, 0.2f, 1, 1);
      sprites[first_border_sprite+2].init(white, -6, 0, 0.2f, 6, 1, 1);
      sprites[first_border_sprite+3].init(white, 6,  0, 0.2f, 6, 1, 1);

      // use the missile texture
      GLuint missile = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/missile.gif");
      for (int i = 0; i != num_missiles; ++i) {
        // create missiles off-screen
        sprites[first_missile_sprite+i].init(missile, 20, 0, 0.0625f, 0.25f, 1, 1);
        sprites[first_missile_sprite+i].is_enabled() = false;
      }

      // use the bomb texture
      GLuint bomb = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/bomb.gif");
      for (int i = 0; i != num_bombs; ++i) {
        // create bombs off-screen
        sprites[first_bomb_sprite+i].init(bomb, 20, 0, 0.0625f, 0.25f, 1, 1);
        sprites[first_bomb_sprite+i].is_enabled() = false;
      }

	  // use the bomb texture
	  GLuint bossbomb = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/bomb.gif");
	  for (int i = 0; i != num_bombs; ++i) {
		  // create bombs off-screen
		  sprites[first_bossbomb_sprite + i].init(bossbomb, 20, 0, 0.0625f, 0.25f, 1, 1);
		  sprites[first_bossbomb_sprite + i].is_enabled() = false;
	  }

      // sounds
      whoosh = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/invaderers/whoosh.wav");
      bang = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/invaderers/bang.wav");
      cur_source = 0;
      alGenSources(num_sound_sources, sources);

      // sundry counters and game state.
      missiles_disabled = 0;
      bombs_disabled = 50;
      invader_velocity = -0.2f;
	  boss_velocity = -0.3f;
      num_lives = 1;
	  boss_lives = 5;
      game_over = false;
      score = 0;
    }

	void boss_init() {
        boss_shader_.init();
		GLuint invaderer = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/invaderer.gif");
		sprites[boss_sprite].init(
			  invaderer, 0, 5.0f, 1.5f, 1.5f, 1, 1
		  );
	}
    // called every frame to move things
    void simulate() {
      if (game_over) {
        return;
      }

      move_ship();

      fire_missiles();

      fire_bombs();

	  fire_boss();

      move_missiles();

      move_bombs();

	  move_bossbombs();

	  ship_collide();

	  boss_collide();

	  collider();

	  invaders_collide();

      move_invaders(0, invader_velocity);

	  move_boss(boss_velocity, 0);

      sprite &border = sprites[first_border_sprite];

    }

    // this is called to draw the world
    void draw_world(int x, int y, int w, int h) {
      simulate();

      // set a viewport - includes whole window area
      glViewport(x, y, w, h);

      // clear the background to black
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // don't allow Z buffer depth testing (closer objects are always drawn in front of far ones)
      glDisable(GL_DEPTH_TEST);

      // allow alpha blend (transparency when alpha channel is 0)
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // draw all the sprites
      for (int i = 0; i != num_sprites; ++i) {
        sprites[i].render(texture_shader_, cameraToWorld);
      }

	  // time simulation
	  int frame = get_frame_number();
	  if (game_over) { frame = 0; }
	  if (frame % 30 == 1) {
		  timer++;
	  }

	  // refresh invaders
	  if (frame % 60 == 2) {
		  GLuint invaderer = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/invaderer.gif");
		  for (int j = 0; j != num_first; ++j) {
			  refresher++;
			  if (refresher==num_invaderers) {
				  refresher = 0;
			  }
			  if (boss_exist) {
				  sprites[first_invaderer_sprite + refresher].init(
					  invaderer, (float)randomizer.get(-4.0f, 4.0f), 4.0f, 0.5f, 0.5f, 1, 1
				  );
				  collider();
			  }
			  else
			  {
				  sprites[first_invaderer_sprite + refresher].init(
					  invaderer, (float)randomizer.get(-4.0f, 4.0f), 6.0f, 0.5f, 0.5f, 1, 1
				  );
				  collider();
			  }
		  }
      }

	  // draw boss invaders
	  if (frame % 150 == 149 && !boss_exist) {
		  boss_init();
		  boss_exist = true;
	  }

	  if (boss_lives == 0) {
		  game_over = true;
		  sprites[game_over_sprite].translate(-20, 0);
	  }

      char score_text[32];
      sprintf(score_text, "score: %d", score);
      draw_text(texture_shader_, -1.75f, 2, 1.0f/256, score_text);

	  char timer_text[32];
	  sprintf(timer_text, "time: %d", timer);
	  draw_text(texture_shader_, -1.75f, 1.8f, 1.0f/256, timer_text);


      // move the listener with the camera
      vec4 &cpos = cameraToWorld.w();
      alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
    }
  };
}
