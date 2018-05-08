
// testDlg.cpp : implementation file
//

#include "stdafx.h"
#include "test.h"
#include "testDlg.h"
#include "afxdialogex.h"
#include "plot.h"
#include "fft.h"

#include <iostream>
#include <array>
#include <numeric>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const double pi = 3.1415926535897932384626433832795;
const int    N  = 1024;

enum class state
{
    clean, spectrum, recovered
};

struct harmonic
{
    double a, f, phi;

    harmonic(double a = 1.,
             double f = .1,
             double phi = 0.)
        : a(a)
        , f(f)
        , phi(phi)
    {
    }
};

using harmonics_t = std::array<harmonic, 3>;
using points_ptr  = std::shared_ptr<plot::list_drawable::data_t>;

double      d          = 0.01;
double      energy_fct = 95;
double      mean_fct   = 30;
harmonics_t harmonics; 
points_ptr  data       = std::make_shared<points_ptr::element_type>(N);
points_ptr  noised     = std::make_shared<points_ptr::element_type>(N);
points_ptr  recovered  = std::make_shared<points_ptr::element_type>(N);
points_ptr  fft        = std::make_shared<points_ptr::element_type>(N);
points_ptr  clean_fft  = std::make_shared<points_ptr::element_type>(N);
double      fft_mean   = -100000;
int         x_scale    = 1000;
double      deviation  = 0;
state       cur_state  = state::clean;

plot::layer_drawable main_plot;
plot::layer_drawable fft_plot;
plot::layer_drawable noised_plot;
plot::world_t world;
plot::world_t noised_world;
plot::world_t fft_world;
cmplx cdata[N];

void calc_deviation()
{
    double max_signal = 0;
    for each (auto &v in *data)
    {
        max_signal = max(max_signal, std::abs(v.y));
    }
    double standard_deviation = 0;
    for (size_t i = 0; i < N; i++)
    {
        standard_deviation += (data->at(i).y - recovered->at(i).y) * (data->at(i).y - recovered->at(i).y);
    }
    standard_deviation = std::sqrt(standard_deviation / N);
    deviation = standard_deviation / max_signal;
}

void apply_x_scale()
{
    double max_f = max(harmonics[0].f, max(harmonics[1].f, harmonics[2].f));
    world.xmax = (2 / max_f) * 1.75 * (x_scale / 1000.);
    noised_world.xmax = (2 / max_f) * 1.75 * (x_scale / 1000.);
}

double s(double x)
{
    double result = 0;
    for each (auto h in harmonics)
    {
        result += h.a * std::sin(2 * pi * h.f * x + h.phi);
    }
    return result;
}

double normal_distribution()
{
    double r = 0;
    for (int i = 0; i < 20; i++)
    {
        r += (double(rand()) / RAND_MAX) * 2 - 1;
    }
    return r / 20;
}

double fft_mean_fnc(double x)
{
    return fft_mean * mean_fct;
}

CtestDlg::CtestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CtestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	main_plot.with(
		plot::plot_builder()
        .in_world(&world)
		.with_ticks(plot::palette::pen(RGB(150, 150, 0)))
		.with_x_ticks(0, 10, 3)
		.with_y_ticks(0, 5, 1)
        .with_data(plot::list_drawable::const_data_factory(data), plot::list_drawable::circle_painter(5, plot::palette::brush(RGB(255, 0, 0))), plot::palette::pen(RGB(255, 0, 0), 3))
        .with_data(plot::list_drawable::const_data_factory(recovered), plot::list_drawable::circle_painter(5, plot::palette::brush(RGB(0, 155, 0))), plot::palette::pen(RGB(0, 155, 0), 3))
		.build()
    );
    noised_plot.with(
        plot::plot_builder()
        .in_world(&noised_world)
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 10, 3)
        .with_y_ticks(0, 5, 1)
        .with_data(plot::list_drawable::const_data_factory(noised), plot::list_drawable::circle_painter(5, plot::palette::brush(RGB(0, 0, 255))), plot::palette::pen(RGB(0, 0, 255), 3))
        .build()
    );
	fft_plot.with(
		plot::plot_builder()
        .in_world(&fft_world)
		.with_ticks(plot::palette::pen(RGB(150, 150, 0)))
		.with_x_ticks(0, 10, 3)
		.with_y_ticks(0, 5, 0)
        .with_data(plot::list_drawable::const_data_factory(fft), plot::list_drawable::circle_painter(2, plot::palette::brush(RGB(255, 0, 0))), plot::palette::pen(RGB(255, 0, 0), 2))
        .with_data(plot::list_drawable::const_data_factory(clean_fft), plot::list_drawable::circle_painter(3, plot::palette::brush(RGB(0, 0, 255))), plot::palette::pen(RGB(0, 0, 255), 2))
        .with_function(&fft_mean_fnc, plot::palette::pen(RGB(0, 255, 0), 3))
		.build()
	);
}

void CtestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_G, plot);
    DDX_Control(pDX, IDC_G2, plot_fft);
    DDX_Control(pDX, IDC_G3, plot_noised);
    DDX_Text(pDX, IDC_A1, harmonics[0].a);
    DDX_Text(pDX, IDC_A2, harmonics[1].a);
    DDX_Text(pDX, IDC_A3, harmonics[2].a);
    DDX_Text(pDX, IDC_f1, harmonics[0].f);
    DDX_Text(pDX, IDC_f2, harmonics[1].f);
    DDX_Text(pDX, IDC_f3, harmonics[2].f);
    DDX_Text(pDX, IDC_phi1, harmonics[0].phi);
    DDX_Text(pDX, IDC_phi2, harmonics[1].phi);
    DDX_Text(pDX, IDC_phi3, harmonics[2].phi);
    DDX_Text(pDX, IDC_mean_factor, energy_fct);
    DDX_Text(pDX, IDC_mean_factor2, mean_fct);
    DDX_Text(pDX, IDC_SNR, d);
    DDX_Slider(pDX, IDC_SLIDER1, x_scale);
    DDX_Control(pDX, IDC_SLIDER1, x_scale_slider);
    DDX_Text(pDX, IDC_deviation, deviation);
}

BEGIN_MESSAGE_MAP(CtestDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON1, &CtestDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CtestDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDC_BUTTON3, &CtestDlg::OnBnClickedButton3)
    ON_WM_HSCROLL()
    ON_BN_CLICKED(IDC_BUTTON4, &CtestDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CtestDlg message handlers

BOOL CtestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    UpdateData(FALSE);

    x_scale_slider.SetRange(1, x_scale * 10, TRUE);
    x_scale_slider.SetPos(x_scale);

    UpdateData(TRUE);
    
    OnBnClickedButton1();

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void draw_layer(CWnd &wnd, plot::layer_drawable &ld)
{
    CDC &dc = *wnd.GetDC();
    SetBkMode(dc, TRANSPARENT);

    RECT bounds;
    wnd.GetClientRect(&bounds);
    plot::viewport vp({ bounds.left, bounds.right, bounds.top, bounds.bottom }, {});

    ld.draw(dc, vp);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CtestDlg::OnPaint()
{
	CDialog::OnPaint();

    draw_layer(plot, main_plot);
    draw_layer(plot_fft, fft_plot);
    draw_layer(plot_noised, noised_plot);
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CtestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CtestDlg::OnBnClickedButton1()
{
    UpdateData(TRUE);

    if (cur_state != state::clean)
    {
        OnBnClickedButton3();
    }

    apply_x_scale();

    double max_a = harmonics[0].a + harmonics[1].a + harmonics[2].a;

    world.ymin = -(world.ymax = max_a * 1.1);
    fft_world.xmin = -(fft_world.xmax = 0.5);

    double signal_energy = 0, noise_energy = 0;
    for (int i = 0; i < N; i++)
    {
        double x = i;
        data->at(i) = { x, s(x) };
        signal_energy += data->at(i).y * data->at(i).y;
        noised->at(i) = { x, normal_distribution() };
        noise_energy += noised->at(i).y * noised->at(i).y;
    }

    double alpha = std::sqrt(d * signal_energy / noise_energy);
    for (int i = 0; i < N; i++)
    {
        noised->at(i).y = data->at(i).y + alpha * noised->at(i).y;
        if (std::abs(noised->at(i).y) * 1.1 > noised_world.ymax)
        {
            noised_world.ymin = -(noised_world.ymax = std::abs(noised->at(i).y) * 1.1);
        }
    }

    for (int i = 0; i < N; i++)
    {
        cdata[i] = { noised->at(i).y, 0 };
    }

    fourier(cdata, N, -1);

    double max_val = 0;

    for (int i = 0; i < N; i++)
    {
        int j = (i + (N/2)) % N;
        fft->at(j) = { double(j - (N/2)) * (0.5 / (N/2)),
                       std::sqrt(cdata[i].real * cdata[i].real + cdata[i].image * cdata[i].image)
                     };
        max_val = max(max_val, fft->at(j).y);
    }

    fft_world.ymax = max_val * 1.1;

    cur_state = state::spectrum;

    Invalidate();
}


void CtestDlg::OnBnClickedButton2()
{
    if (cur_state != state::spectrum)
    {
        OnBnClickedButton1();
    }

    apply_x_scale();

    double fft_energy = 0;
    for (int i = 0; i < N; i++)
    {
        fft_energy += fft->at(i).y * fft->at(i).y;
    }

    double cur_energy = 0;
    for (int i = 0; i < N / 2; i++)
    {
        int j = (i + (N / 2)) % N;
        clean_fft->at(j) = fft->at(j);
        clean_fft->at(N - j - 1) = fft->at(N - j - 1);
        cur_energy += 2 * fft->at(j).y * fft->at(j).y;
        if (cur_energy > fft_energy * (energy_fct / 100))
        {
            clean_fft->at(j).y = 0;
            clean_fft->at(N - j - 1).y = 0;
            cdata[i] = { 0, 0 };
            cdata[N - i - 1] = { 0, 0 };
        }
    }

    fourier(cdata, N, 1);

    for (int i = 0; i < N; i++)
    {
        recovered->at(i) = { double(i), cdata[i].real };
        if (std::abs(recovered->at(i).y) * 1.1 > world.ymax)
        {
            world.ymin = -(world.ymax = std::abs(recovered->at(i).y) * 1.1);
        }
    }

    calc_deviation();

    cur_state = state::recovered;

    Invalidate();

    UpdateData(FALSE);
}


void CtestDlg::OnBnClickedButton4()
{
    if (cur_state != state::spectrum)
    {
        OnBnClickedButton1();
    }

    apply_x_scale();

    fft_mean = 0;
    for (int i = 0; i < N; i++)
    {
        fft_mean += fft->at(i).y;
    }
    fft_mean /= N;

    double fft_thr = fft_mean * mean_fct;

    for (int i = 0; i < N; i++)
    {
        int j = (i + (N / 2)) % N;
        clean_fft->at(j) = fft->at(j);
        if (fft->at(j).y < fft_thr)
        {
            clean_fft->at(j).y = 0;
            cdata[i] = { 0, 0 };
        }
    }

    fourier(cdata, N, 1);

    for (int i = 0; i < N; i++)
    {
        recovered->at(i) = { double(i), cdata[i].real };
        if (std::abs(recovered->at(i).y) * 1.1 > world.ymax)
        {
            world.ymin = -(world.ymax = std::abs(recovered->at(i).y) * 1.1);
        }
    }

    calc_deviation();

    cur_state = state::recovered;

    Invalidate();

    UpdateData(FALSE);
}


void CtestDlg::OnBnClickedButton3()
{
    apply_x_scale();

    for (size_t i = 0; i < N; i++)
    {
        data->at(i) = {};
        recovered->at(i) = {};
        noised->at(i) = {};
        fft->at(i) = {};
        clean_fft->at(i) = {};
    }
    fft_mean = -100000;
    cur_state = state::clean;

    Invalidate();
}


void CtestDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    UpdateData(TRUE);
    apply_x_scale();
    Invalidate();
}
