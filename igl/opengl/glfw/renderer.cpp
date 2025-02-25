#include "igl/opengl/glfw/renderer.h"
#include <GLFW/glfw3.h>
#include <igl/unproject_onto_mesh.h>
#include "igl/look_at.h"

Renderer::Renderer() : selected_core_index(0),
next_core_id(2),
change_camera(0)
{
	core_list.emplace_back(igl::opengl::ViewerCore());
	core_list.front().id = 1;
	// C-style callbacks
	callback_init = nullptr;
	callback_pre_draw = nullptr;
	callback_post_draw = nullptr;
	callback_mouse_down = nullptr;
	callback_mouse_up = nullptr;
	callback_mouse_move = nullptr;
	callback_mouse_scroll = nullptr;
	callback_key_down = nullptr;
	callback_key_up = nullptr;

	callback_init_data = nullptr;
	callback_pre_draw_data = nullptr;
	callback_post_draw_data = nullptr;
	callback_mouse_down_data = nullptr;
	callback_mouse_up_data = nullptr;
	callback_mouse_move_data = nullptr;
	callback_mouse_scroll_data = nullptr;
	callback_key_down_data = nullptr;
	callback_key_up_data = nullptr;
	//highdpi = 1;

	xold = 0;
	yold = 0;
}

IGL_INLINE void Renderer::draw(GLFWwindow* window)
{
	using namespace std;
	using namespace Eigen;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	int width_window, height_window;
	glfwGetWindowSize(window, &width_window, &height_window);

	auto highdpi_tmp = (width_window == 0 || width == 0) ? highdpi : (width / width_window);

	if (fabs(highdpi_tmp - highdpi) > 1e-8)
	{
		post_resize(window, width, height);
		highdpi = highdpi_tmp;
	}

	for (auto& core : core_list)
	{
		core.clear_framebuffers();
	}
	int coreIndx = 1;
	if (menu)
	{
		menu->pre_draw();
		menu->callback_draw_viewer_menu();
	}
	for (auto& core : core_list)
	{
		int indx = 0;
		for (auto& mesh : scn->data_list)
		{

			if (mesh.is_visible & core.id)
			{// for kinematic chain change scn->MakeTrans to parent matrix
				//Project comment
				//if (selected_core_index == 0) {
				if (change_camera != 0) {
					//camera translation- get the translation of snake head
					//camera eye - get the direction of the snake (target pose, changed by the keys) + playing with phi/2 for better view position
					//camera up- by phi/2 in the currect postion of the direction(for example moving right(speed,0,0), than camera up is(phi/2,0,0)

					if (scn->target_pose(2) > 0) { // when moving right
						core.camera_translation = -scn->snake_links[scn->snake_links.size() - 1].GetTranslation().cast<float>();
						core.camera_eye = scn->target_pose.cast<float>() + Eigen::Vector3f(-M_PI * 1.25, 0, M_PI / 2);
						core.camera_up = Eigen::Vector3f(M_PI / 2, 0, 0);
					}
					else if (scn->target_pose(2) < 0) { // when moving left
						core.camera_translation = -scn->snake_links[scn->snake_links.size() - 1].GetTranslation().cast<float>();
						core.camera_eye = scn->target_pose.cast<float>() + Eigen::Vector3f(M_PI * 1.25, 0, M_PI / 2);
						core.camera_up = Eigen::Vector3f(-M_PI, 0, 0);
					}
					else if (scn->target_pose(1) > 0) { // when moving up
						core.camera_translation = -scn->snake_links[scn->snake_links.size() - 1].GetTranslation().cast<float>();
						core.camera_eye = scn->target_pose.cast<float>() + Eigen::Vector3f(0, -M_PI * 1.25, M_PI / 2);
						core.camera_up = Eigen::Vector3f(0, M_PI, 0);
					}
					else if (scn->target_pose(1) < 0) { //when moving down
						core.camera_translation = -scn->snake_links[scn->snake_links.size() - 1].GetTranslation().cast<float>();
						core.camera_eye = scn->target_pose.cast<float>() + Eigen::Vector3f(0, M_PI * 1.25, M_PI / 2);
						core.camera_up = Eigen::Vector3f(0, -M_PI, 0);
					}
				}
				else {
					core.camera_translation = prev_camera_translation;
					core.camera_eye = prev_camera_eye;
					core.camera_up = prev_camera_up;
				}
				//end comment Project

				core.draw(scn->MakeTransScale() * scn->CalcParentsTrans(indx).cast<float>(), mesh);
			}
			indx++;
		}
	}
	if (menu)
	{
		menu->post_draw();

	}

}

void Renderer::SetScene(igl::opengl::glfw::Viewer* viewer)
{
	scn = viewer;
}

IGL_INLINE void Renderer::init(igl::opengl::glfw::Viewer* viewer, int coresNum, igl::opengl::glfw::imgui::ImGuiMenu* _menu)
{
	scn = viewer;

	doubleVariable = 0;
	core().init();
	menu = _menu;
	core().align_camera_center(scn->data().V, scn->data().F);

	if (coresNum > 1)
	{
		int width = core().viewport[2];
		int height = core().viewport[3];

		core().viewport = Eigen::Vector4f(0, 0, width / 4, height);
		left_view = core_list[0].id;
		right_view = append_core(Eigen::Vector4f(width / 4, 0, width * 3 / 4, height));
		core_index(right_view - 1);
		for (size_t i = 0; i < scn->data_list.size(); i++)
		{
			core().toggle(scn->data_list[i].show_faces);
			core().toggle(scn->data_list[i].show_lines);
			core().toggle(scn->data_list[i].show_texture);

			//Ass 2 comment
			core().toggle(scn->data_list[i].show_overlay);
			core().toggle(scn->data_list[i].show_overlay_depth);
			//end comment Ass 2

		}

		core_index(left_view - 1);
		//Project comment
		prev_camera_translation = core().camera_translation;
		prev_camera_eye = core().camera_eye;
		prev_camera_up = core().camera_up;
		//Project comment end
	}

	if (menu)
	{
		//project
		//removed the original menu
		menu->callback_draw_viewer_menu = [&]()
		{
			// Draw parent menu content
			//menu->draw_viewer_menu(scn, core_list);
			menu->callback_draw_custom_window();
		};
		//project end
	}
}

void Renderer::UpdatePosition(double xpos, double ypos) {
	xrel = xold - xpos;
	yrel = yold - ypos;
	xold = xpos;
	yold = ypos;
}

void Renderer::MouseProcessing(int button) {
	if (scn->isPicked)
	{
		if (button == 1)
		{
			float near = core().camera_dnear, far = core().camera_dfar, angle = core().camera_view_angle;
			Eigen::Matrix4f tmpM = core().proj;
			double xToMove = -(double)xrel / core().viewport[3] * (z + 2 * near) * (far) / (far + 2 * near) * 2.0 * tanf(angle / 360 * M_PI) / (core().camera_zoom * core().camera_base_zoom);
			double yToMove = (double)yrel / core().viewport[3] * (z + 2 * near) * (far) / (far + 2 * near) * 2.0 * tanf(angle / 360 * M_PI) / (core().camera_zoom * core().camera_base_zoom);

			//Project comment
			scn->data().MyTranslateInSystem(scn->GetRotation(), Eigen::Vector3d(xToMove, 0, 0));
			scn->data().MyTranslateInSystem(scn->GetRotation(), Eigen::Vector3d(0, yToMove, 0));
			//end comment Project
			scn->WhenTranslate();

		}
		else
		{
			//Project comment
			scn->data().RotateInSystem(Eigen::Vector3d(1, 0, 0), yrel / 100.0);
			scn->data().RotateInSystem(Eigen::Vector3d(0, 1, 0), xrel / 100.0);
			//end comment Project
		}
	}
	else
	{
		if (button == 1)
		{
			float near = core().camera_dnear, far = core().camera_dfar, angle = core().camera_view_angle;
			float z = far + 0.5f * (near - far);


			double xToMove = -(double)xrel / core().viewport[3] * far / z * near * 2.0f * tanf(angle / 360 * M_PI) / (core().camera_zoom * core().camera_base_zoom);
			double yToMove = (double)yrel / core().viewport[3] * far / z * near * 2.0f * tanf(angle / 360 * M_PI) / (core().camera_zoom * core().camera_base_zoom);

			//if fixing to tranlate in system, sometimes all scene move to different sides then actually mouse moved, so not to change
			scn->MyTranslate(Eigen::Vector3d(xToMove, 0, 0), true);
			scn->MyTranslate(Eigen::Vector3d(0, yToMove, 0), true);
			//comment-if fixing to tranlate in system, sometimes all scene move to different sides then actually mouse moved,
			//so not to change -end

		}
		else
		{
			//Project comment
			scn->RotateInSystem(Eigen::Vector3d(1, 0, 0), yrel / 100.0);
			scn->RotateInSystem(Eigen::Vector3d(0, 1, 0), xrel / 100.0);
			//end comment Project
		}
	}
}

void Renderer::TranslateCamera(Eigen::Vector3f amt) {
	core().camera_translation += amt;
}

void Renderer::RotateCamera(float amtX, float amtY) {
	core().camera_eye = core().camera_eye + Eigen::Vector3f(0, amtY, 0);
	Eigen::Matrix3f Mat;
	Mat << cos(amtY), 0, sin(amtY), 0, 1, 0, -sin(amtY), 0, cos(amtY);
	core().camera_eye = Mat * core().camera_eye;

}

Renderer::~Renderer() { }


//Ass 2 comment
void Renderer::changeMovingDirection(int dir) {
	//having a velocity for each direction the object is moving
	double velocity = 0.004;
	switch (dir) {
	case GLFW_KEY_UP:
		GetScene()->data_list[GetScene()->moving_index].moveDir = Eigen::Vector3d(0, velocity, 0);
		break;
	case GLFW_KEY_DOWN:
		GetScene()->data_list[GetScene()->moving_index].moveDir = Eigen::Vector3d(0, -velocity, 0);
		break;
	case GLFW_KEY_LEFT:
		GetScene()->data_list[GetScene()->moving_index].moveDir = Eigen::Vector3d(-velocity, 0, 0);
		break;
	case GLFW_KEY_RIGHT:
		GetScene()->data_list[GetScene()->moving_index].moveDir = Eigen::Vector3d(velocity, 0, 0);
		break;
	case 'w':
		GetScene()->data_list[GetScene()->moving_index].moveDir = Eigen::Vector3d(0, 0, -velocity);
		break;
	case 's':
		GetScene()->data_list[GetScene()->moving_index].moveDir = Eigen::Vector3d(0, 0, velocity);
		break;
	default: break;
	}
}
//end comment Ass 2

//Ass3
void Renderer::changeRotateAxis(int rotate) {
	if (scn->current_picked != -1) {
		switch (rotate)
		{
		case GLFW_KEY_UP:
			scn->data().MyRotate(Eigen::Vector3d(1, 0, 0), -0.1, false);
			break;
		case GLFW_KEY_DOWN:
			scn->data().MyRotate(Eigen::Vector3d(1, 0, 0), 0.1, false);
			break;
		case GLFW_KEY_LEFT:
			scn->data().MyRotate(Eigen::Vector3d(0, 0, 1), -0.1, true);
			break;
		case GLFW_KEY_RIGHT:
			scn->data().MyRotate(Eigen::Vector3d(0, 0, 1), 0.1, true);
			break;
		default:
			break;
		}

	}
	else {
		switch (rotate)
		{
		case GLFW_KEY_UP:
			scn->MyRotate(Eigen::Vector3d(1, 0, 0), -0.1, false);
			break;
		case GLFW_KEY_DOWN:
			scn->MyRotate(Eigen::Vector3d(1, 0, 0), 0.1, false);
			break;
		case GLFW_KEY_LEFT:
			scn->MyRotate(Eigen::Vector3d(0, 0, 1), -0.1, true);
			break;
		case GLFW_KEY_RIGHT:
			scn->MyRotate(Eigen::Vector3d(0, 0, 1), 0.1, true);
			break;
		default:
			break;
		}

	}
}
//end Ass3



double Renderer::Picking(double newx, double newy)
{
	int fid;
	Eigen::Vector3f bc;
	double x = newx;
	double y = core().viewport(3) - newy;

	Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

	igl::look_at(core().camera_eye, core().camera_center, core().camera_up, view);
	//std::cout << "view matrix\n" << view << std::endl;
	view = view * (core().trackball_angle * Eigen::Scaling(core().camera_zoom * core().camera_base_zoom)
		* Eigen::Translation3f(core().camera_translation + core().camera_base_translation)).matrix() * scn->MakeTransScale() * scn->CalcParentsTrans(scn->selected_data_index).cast<float>() * scn->data().MakeTransScale();
	bool picked = igl::unproject_onto_mesh(Eigen::Vector2f(x, y), view,
		core().proj, core().viewport, scn->data().V, scn->data().F, fid, bc);
	scn->isPicked = scn->isPicked | picked;
	if (picked)
	{
		Eigen::Vector3i face = scn->data().F.row(fid);
		Eigen::Matrix3d vertices;
		Eigen::Vector4f p, pp;

		vertices.col(0) = scn->data().V.row(face(0));
		vertices.col(1) = scn->data().V.row(face(1));
		vertices.col(2) = scn->data().V.row(face(2));

		p << vertices.cast<float>() * bc, 1;
		p = view * p;
		pp = core().proj * p;
		//glReadPixels(x,  y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
		z = pp(2);
		return p(2);

	}
	return 0;
}

IGL_INLINE void Renderer::resize(GLFWwindow* window, int w, int h)
{
	if (window) {
		glfwSetWindowSize(window, w / highdpi, h / highdpi);
	}
	post_resize(window, w, h);
}

IGL_INLINE void Renderer::post_resize(GLFWwindow* window, int w, int h)
{
	if (core_list.size() == 1)
	{

		core().viewport = Eigen::Vector4f(0, 0, w, h);
	}
	else
	{
		// It is up to the user to define the behavior of the post_resize() function
		// when there are multiple viewports (through the `callback_post_resize` callback)
		core(left_view).viewport = Eigen::Vector4f(0, 0, w / 4, h);
		core(right_view).viewport = Eigen::Vector4f(w / 4, 0, w - (w / 4), h);

	}
	//for (unsigned int i = 0; i < plugins.size(); ++i)
	//{
	//	plugins[i]->post_resize(w, h);
	//}
	if (callback_post_resize)
	{
		callback_post_resize(window, w, h);
	}
}

IGL_INLINE igl::opengl::ViewerCore& Renderer::core(unsigned core_id /*= 0*/)
{
	assert(!core_list.empty() && "core_list should never be empty");
	int core_index;
	if (core_id == 0)
		core_index = selected_core_index;
	else
		core_index = this->core_index(core_id);
	assert((core_index >= 0 && core_index < core_list.size()) && "selected_core_index should be in bounds");
	return core_list[core_index];
}

IGL_INLINE const igl::opengl::ViewerCore& Renderer::core(unsigned core_id /*= 0*/) const
{
	assert(!core_list.empty() && "core_list should never be empty");
	int core_index;
	if (core_id == 0)
		core_index = selected_core_index;
	else
		core_index = this->core_index(core_id);
	assert((core_index >= 0 && core_index < core_list.size()) && "selected_core_index should be in bounds");
	return core_list[core_index];
}

IGL_INLINE bool Renderer::erase_core(const size_t index)
{
	assert((index >= 0 && index < core_list.size()) && "index should be in bounds");
	//assert(data_list.size() >= 1);
	if (core_list.size() == 1)
	{
		// Cannot remove last viewport
		return false;
	}
	core_list[index].shut(); // does nothing
	core_list.erase(core_list.begin() + index);
	if (selected_core_index >= index && selected_core_index > 0)
	{
		selected_core_index--;
	}
	return true;
}

IGL_INLINE size_t Renderer::core_index(const int id) const {
	for (size_t i = 0; i < core_list.size(); ++i)
	{
		if (core_list[i].id == id)
			return i;
	}
	return 0;
}

IGL_INLINE int Renderer::append_core(Eigen::Vector4f viewport, bool append_empty /*= false*/)
{
	core_list.push_back(core()); // copies the previous active core and only changes the viewport
	core_list.back().viewport = viewport;
	core_list.back().id = next_core_id;
	next_core_id <<= 1;
	if (!append_empty)
	{
		for (auto& data : scn->data_list)
		{
			data.set_visible(true, core_list.back().id);
			//data.copy_options(core(), core_list.back());
		}
	}
	selected_core_index = core_list.size() - 1;
	return core_list.back().id;
}