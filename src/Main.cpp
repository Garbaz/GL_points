#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "Main.hpp"

#define PRINT_DEBUG true

const float FIELD_OF_VIEW_HORIZONTAL = 100.0;

const float CAMERA_MOUSE_SENS = 5.0 * glm::radians(0.022);
const float FLY_SPEED = 0.1;

const int NUMBER_OF_PARTICLES = 100000;

GLFWwindow *window;
glm::ivec2 viewport_size;

bool mouse_captured = false;
glm::vec2 prev_cursor_pos = glm::vec2(0, 0);

double camera_angle_vertical = 0;
double camera_angle_horizontal = 0;
glm::vec3 camera_position = glm::vec3(0, 0, 0);
glm::vec3 camera_direction = glm::vec3(0, 0, 1);
glm::mat4 view_matrix;
bool view_matrix_dirty;

glm::ivec2 input_movement = glm::ivec2(0, 0);

void update_view_matrix() {
   view_matrix = glm::lookAt(camera_position, camera_position + camera_direction, glm::vec3(0, 1, 0));
   view_matrix_dirty = true;
}
void update_camera_position(glm::vec3 new_camera_position) {
   camera_position = new_camera_position;
   update_view_matrix();
}
void update_camera_direction(glm::vec3 new_camera_direction) {
   camera_direction = new_camera_direction;
   update_view_matrix();
}

bool pause_simulation = true;

struct Particle {
   glm::vec3 pos;
   glm::vec3 vel;
   glm::vec3 color;
};
Particle particles[NUMBER_OF_PARTICLES];

/*PHYSICS STEP:*/
void update_particles(float dt) {
   for (int i = 0; i < NUMBER_OF_PARTICLES; i++) {
      Particle *p = &particles[i];
      // glm::vec3 acc = -glm::linearRand(0.1f,0.3f) * p->pos;
      // p->vel += dt * acc;
      p->pos += dt * p->vel;

      if (glm::abs(p->pos.x) >= 10.0 && p->pos.x * p->vel.x > 0) {
         p->vel.x = -p->vel.x;
      }
      if (glm::abs(p->pos.y) >= 10.0 && p->pos.y * p->vel.y > 0) {
         p->vel.y = -p->vel.y;
      }
      if (glm::abs(p->pos.z) >= 10.0 && p->pos.z * p->vel.z > 0) {
         p->vel.z = -p->vel.z;
      }
   }
}

int main() {
   srand(time(0));

   init_glfw();
   init_glew();

   float range[2];
   glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE,range);
   std::cout << range[0] << " -- " << range[1] << std::endl;

#if PRINT_DEBUG
   glEnable(GL_DEBUG_OUTPUT);
   glDebugMessageCallback(message_callback, 0);
#endif

   glEnable(GL_PROGRAM_POINT_SIZE);
   // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   // glEnable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);

   GLuint vao;
   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);

   for (int i = 0; i < NUMBER_OF_PARTICLES; i++) {
      Particle p;
      //p.pos = glm::linearRand(glm::vec3(-1, -1, -1), glm::vec3(1, 1, 1));
      p.pos = glm::vec3(0);
      p.vel = glm::sphericalRand(10.0);
      p.color = random_saturated_color();

      particles[i] = p;
   }

   GLuint vbo;
   glGenBuffers(1, &vbo);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(particles), particles, GL_DYNAMIC_DRAW);

   GLuint vertex_shader = compile_shader("shaders/point.vert", GL_VERTEX_SHADER);
   GLuint fragment_shader = compile_shader("shaders/point.frag", GL_FRAGMENT_SHADER);
   GLuint shader_program = glCreateProgram();
   glAttachShader(shader_program, vertex_shader);
   glAttachShader(shader_program, fragment_shader);
   glBindFragDataLocation(shader_program, 0, "out_color");
   glLinkProgram(shader_program);
   glUseProgram(shader_program);
   GLint attrib_loc_position = glGetAttribLocation(shader_program, "position");
   glEnableVertexAttribArray(attrib_loc_position);
   glVertexAttribPointer(attrib_loc_position, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, pos));
   GLint attrib_loc_color = glGetAttribLocation(shader_program, "color");
   glEnableVertexAttribArray(attrib_loc_color);
   glVertexAttribPointer(attrib_loc_color, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, color));

   float aspect = float(viewport_size.x) / float(viewport_size.y);
   float fovy = FIELD_OF_VIEW_HORIZONTAL / aspect;
   glm::mat4 proj = glm::perspective(glm::radians(fovy), aspect, 0.01f, 100.0f);

   update_view_matrix();

   GLint uniform_time = glGetUniformLocation(shader_program, "time");
   GLint uniform_deltatime = glGetUniformLocation(shader_program, "deltatime");
   GLint uniform_view = glGetUniformLocation(shader_program, "view");
   GLint uniform_proj = glGetUniformLocation(shader_program, "proj");
   glUniformMatrix4fv(uniform_proj, 1, GL_FALSE, glm::value_ptr(proj));

   glfwSetKeyCallback(window, key_callback);
   glfwSetCursorPosCallback(window, cursor_callback);

   if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
   }

   double start_time = glfwGetTime();
   double last_frame_time = start_time;

   while (!glfwWindowShouldClose(window)) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      /* Camera movement */
      if (input_movement.x != 0 || input_movement.y != 0) {
         glm::vec3 dir_right = glm::vec3(camera_direction.z, 0, -camera_direction.x);
         //glm::vec3 dir_forward = glm::vec3(camera_direction.x, 0, camera_direction.z);
         glm::vec3 dir_forward = camera_direction;
         glm::vec3 move = FLY_SPEED * (float(input_movement.x) * dir_right + float(input_movement.y) * dir_forward);
         update_camera_position(camera_position + move);
      }
      if (view_matrix_dirty) {
         glUniformMatrix4fv(uniform_view, 1, GL_FALSE, glm::value_ptr(view_matrix));
         view_matrix_dirty = false;
      }

      /*Update time & deltatime*/
      double frame_time = glfwGetTime();
      double time = frame_time - start_time;
      double deltatime = frame_time - last_frame_time;
      last_frame_time = frame_time;
      glUniform1f(uniform_time, time);
      glUniform1f(uniform_deltatime, deltatime);

      /*Physics simulation step*/
      if (!pause_simulation) {
         update_particles(deltatime);
         glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(particles), particles);
      }

      /*Draw*/
      glDrawArrays(GL_POINTS, 0, NUMBER_OF_PARTICLES);

      glfwSwapBuffers(window);
      glfwPollEvents();
   }

   glfwTerminate();
   return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
   if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      exit(0);
   } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
      if (mouse_captured) {
         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
         mouse_captured = false;
      } else {
         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
         mouse_captured = true;
      }
   } else if (key == GLFW_KEY_ENTER && action == GLFW_PRESS){
      pause_simulation = !pause_simulation;
   } else if (key == GLFW_KEY_W) {
      if (action == GLFW_PRESS) {
         input_movement.y++;
      } else if (action == GLFW_RELEASE) {
         input_movement.y--;
      }
   } else if (key == GLFW_KEY_S) {
      if (action == GLFW_PRESS) {
         input_movement.y--;
      } else if (action == GLFW_RELEASE) {
         input_movement.y++;
      }
   } else if (key == GLFW_KEY_A) {
      if (action == GLFW_PRESS) {
         input_movement.x++;
      } else if (action == GLFW_RELEASE) {
         input_movement.x--;
      }
   } else if (key == GLFW_KEY_D) {
      if (action == GLFW_PRESS) {
         input_movement.x--;
      } else if (action == GLFW_RELEASE) {
         input_movement.x++;
      }
   }
}

void cursor_callback(GLFWwindow *window, double xpos, double ypos) {
   if (mouse_captured) {
      camera_angle_horizontal -= CAMERA_MOUSE_SENS * (xpos - prev_cursor_pos.x);
      camera_angle_vertical -= CAMERA_MOUSE_SENS * (ypos - prev_cursor_pos.y);
      if (camera_angle_vertical > 0.99 * glm::half_pi<double>()) {
         camera_angle_vertical = 0.99 * glm::half_pi<double>();
      } else if (camera_angle_vertical < -0.99 * glm::half_pi<double>()) {
         camera_angle_vertical = -0.99 * glm::half_pi<double>();
      }
      update_camera_direction(glm::vec3(glm::cos(camera_angle_vertical) * glm::sin(camera_angle_horizontal),
                                        glm::sin(camera_angle_vertical),
                                        glm::cos(camera_angle_vertical) * glm::cos(camera_angle_horizontal)));
   }
   prev_cursor_pos.x = xpos;
   prev_cursor_pos.y = ypos;
}

void init_glfw() {
   if (!glfwInit()) {
      std::cerr << "Failed to initialize GLFW" << std::endl;
      exit(-1);
   }
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
   GLFWmonitor *monitor = glfwGetPrimaryMonitor();
   const GLFWvidmode *vidmode = glfwGetVideoMode(monitor);
   glfwWindowHint(GLFW_RED_BITS, vidmode->redBits);
   glfwWindowHint(GLFW_GREEN_BITS, vidmode->greenBits);
   glfwWindowHint(GLFW_BLUE_BITS, vidmode->blueBits);
   glfwWindowHint(GLFW_REFRESH_RATE, vidmode->refreshRate);
   window = glfwCreateWindow(vidmode->width, vidmode->height, "RaytraceGL", monitor, NULL);
   //window = glfwCreateWindow(1200, 675, "GL Points", NULL, NULL);

   glfwGetFramebufferSize(window, &viewport_size.x, &viewport_size.y);

   glfwMakeContextCurrent(window);
}

void init_glew() {
   glewExperimental = GL_TRUE;
   GLenum err = glewInit();
   if (GLEW_OK != err) {
      std::cerr << "glew init error: " << glewGetErrorString(err) << std::endl;
      exit(-1);
   }
}

GLuint compile_shader(std::string filename, GLenum shader_type) {
   std::string source = read_file(filename);
   const char *source_cstr = source.c_str();

   GLuint shader = glCreateShader(shader_type);
   glShaderSource(shader, 1, &source_cstr, NULL);
   glCompileShader(shader);
   GLint status;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
#if PRINT_DEBUG
   std::cerr << "Shader \"" << filename << "\" compiled " << (status == GL_TRUE ? "successful" : "unsuccessful") << "." << std::endl;

   GLint info_log_length;
   glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
   if (info_log_length > 0) {
      char info_log[info_log_length];
      glGetShaderInfoLog(shader, info_log_length, NULL, info_log);
      std::cerr << "Shader compolation log:" << std::endl;
      std::cerr << info_log << std::endl;
   }
#endif
   if (status != GL_TRUE) {
      exit(-1);
   }
   return shader;
}

std::string read_file(std::string filename) {
   std::ifstream f(filename);
   if (f) {
      std::ostringstream ss;
      ss << f.rdbuf();
      return ss.str();
   }
   return nullptr;
}

void GLAPIENTRY message_callback(GLenum source,
                                 GLenum type,
                                 GLuint id,
                                 GLenum severity,
                                 GLsizei length,
                                 const GLchar *message,
                                 const void *user_param) {
   fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
           type, severity, message);
}

glm::vec3 random_saturated_color() {
   float m = glm::linearRand(0, 1);
   switch (rand() % 6) {
      case 0:
         return glm::vec3(1, 0, m);
      case 1:
         return glm::vec3(1, m, 0);
      case 2:
         return glm::vec3(0, m, 1);
      case 3:
         return glm::vec3(0, 1, m);
      case 4:
         return glm::vec3(m, 1, 0);
      case 5:
         return glm::vec3(m, 0, 1);
   }
}