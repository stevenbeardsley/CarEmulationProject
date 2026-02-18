using System;
using System.ComponentModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using SimulationPlatform.Models;
using Windows.Foundation;

namespace SimulationPlatform.Pages
{
    public sealed partial class TrackPage : Page, INotifyPropertyChanged
    {
        private readonly AppModel m_model;
        public event PropertyChangedEventHandler? PropertyChanged;

        private string m_speed = string.Empty;
        private int m_gear;
        private string m_rpms = string.Empty;
        private string m_engineTemp = string.Empty;

        private double _acceleration = 25;
        private double _speedValue;
        private double _oneCopyHeight = 0;

        public Brush m_colour
        {
            get; set;
        }

        private bool _laneScrollStarted = false;

        // =========================
        // RPM DIAL (tachometer)
        // =========================
        private const double MaxRpm = 6000.0; // TODO: change
        private bool _rpmDialBuilt = false;

        private const double StartAngleDeg = 180.0;  
        private const double SweepDeg = 180.0;       

        // Redline begins at 85% of max
        private const double RedlineStartRatio = 0.85;

        public double SpeedValue
        {
            get => _speedValue;
            set
            {
                if (Math.Abs(_speedValue - value) > 0.001)
                {
                    _speedValue = value;
                    OnPropertyChanged(nameof(SpeedValue));
                    UpdateRoadScrollSpeed();
                }
            }
        }

        public double Acceleration
        {
            get => _acceleration;
            set
            {
                if (Math.Abs(_acceleration - value) > 0.001)
                {
                    _acceleration = value;
                    OnPropertyChanged(nameof(Acceleration));
                    OnPropertyChanged(nameof(AccelerationText));
                }
            }
        }

        public string AccelerationText => $"{Acceleration:0}%";

        public bool m_downShiftEnabled => m_gear != 0;

        public bool m_upShiftEnabled
        {
            get => m_gear < m_model.CarConfig.GearsCount;
        }

        public int GearValue
        {
            get => m_gear;
            set
            {
                if (m_gear != value)
                {
                    m_gear = value;

                    OnPropertyChanged(nameof(GearValue));
                    OnPropertyChanged(nameof(Gear));
                    OnPropertyChanged(nameof(m_downShiftEnabled));
                    OnPropertyChanged(nameof(m_upShiftEnabled));
                }
            }
        }

        // UI-facing string
        public string Gear => m_gear.ToString();

        public string Speed
        {
            get => m_speed;
            set
            {
                if (m_speed != value)
                {
                    m_speed = value;
                    OnPropertyChanged(nameof(Speed));
                }
            }
        }

        // Keep your existing binding mechanics: RPM is still a string property.
        public string Rpm
        {
            get => m_rpms;
            set
            {
                if (m_rpms != value)
                {
                    m_rpms = value;
                    OnPropertyChanged(nameof(Rpm));
                }
            }
        }

        public string EngineTemp
        {
            get => m_engineTemp;
            set
            {
                if (m_engineTemp != value)
                {
                    m_engineTemp = value;
                    OnPropertyChanged(nameof(EngineTemp));
                }
            }
        }

        public TrackPage()
        {
            InitializeComponent();
            m_model = App.m_model;

            // slider commit on release only (registered once)
            AccelerationSlider.AddHandler(
                UIElement.PointerReleasedEvent,
                new Microsoft.UI.Xaml.Input.PointerEventHandler(AccelerationSlider_PointerReleased),
                handledEventsToo: true);

            AccelerationSlider.AddHandler(
                UIElement.PointerCaptureLostEvent,
                new Microsoft.UI.Xaml.Input.PointerEventHandler(AccelerationSlider_PointerCaptureLost),
                handledEventsToo: true);

            // clipping
            LaneViewport.SizeChanged += LaneViewport_SizeChanged;

            Loaded += (_, __) =>
            {
                ApplyLaneClip();
                SetupSeamlessLaneScroll();     // sets To = _oneCopyHeight
                StartLaneScrollIfNeeded();     // begins storyboard controllably once
                UpdateRoadScrollSpeed();       // applies correct ratio/pause/resume

                // Draw RPM dial once XAML is ready (uses current model value)
                UpdateRpmGauge(App.m_model.m_carData.Rpms);
            };
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            SpeedValue = App.m_model.m_carData.Speed;
            Speed = SpeedValue.ToString();

            Rpm = App.m_model.m_carData.Rpms.ToString();
            UpdateRpmGauge(App.m_model.m_carData.Rpms);

            GearValue = App.m_model.m_carData.Gear;

            App.m_model.m_webSocketController.CarDataReceived += UpdateCarData;

            m_colour = m_model.CarConfig.Colour;
            UpdateRoadScrollSpeed();
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            App.m_model.m_webSocketController.CarDataReceived -= UpdateCarData;
        }

        private void OnPropertyChanged(string propertyName)
            => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));

        private async void accelerationSelected()
        {
            try { await m_model.VehicleController.SetThrottleAsync(Acceleration); }
            catch (Exception ex) { System.Diagnostics.Debug.WriteLine(ex.Message); }
        }

        private void UpdateCarData(CarData carData)
        {
            DispatcherQueue.TryEnqueue(() =>
            {
                SpeedValue = carData.Speed;           // drives animation + raises property changed
                Speed = carData.Speed.ToString();
                EngineTemp = carData.EngineTemp.ToString();
                // Keep string binding for text readout
                Rpm = carData.Rpms.ToString();

                // Update dial using numeric value from model/telemetry
                UpdateRpmGauge(carData.Rpms);

                GearValue = carData.Gear;
            });
        }

        private async void GearShiftDown_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await m_model.VehicleController.ShiftDownAsync();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine(ex.Message);
            }
        }

        private async void GearShiftUp_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await m_model.VehicleController.ShiftUpAsync();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine(ex.Message);
            }
        }

        private void AccelerationSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (Math.Abs(Acceleration - e.NewValue) > 0.001)
                Acceleration = e.NewValue;
        }

        private void AccelerationSlider_PointerReleased(object sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
            => accelerationSelected();

        private void AccelerationSlider_PointerCaptureLost(object sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
            => accelerationSelected();

        private void LaneViewport_SizeChanged(object sender, SizeChangedEventArgs e)
            => ApplyLaneClip();

        private void ApplyLaneClip()
        {
            var w = LaneViewport.ActualWidth;
            var h = LaneViewport.ActualHeight;

            if (w > 0 && h > 0)
            {
                LaneViewport.Clip = new Microsoft.UI.Xaml.Media.RectangleGeometry
                {
                    Rect = new Windows.Foundation.Rect(0, 0, w, h)
                };
            }
        }

        private void SetupSeamlessLaneScroll()
        {
            LaneScrollContent.UpdateLayout();

            // Find first StackPanel copy directly
            FrameworkElement? firstCopy = null;
            foreach (var child in LaneScrollContent.Children)
            {
                if (child is FrameworkElement fe)
                {
                    firstCopy = fe;
                    break;
                }
            }
            if (firstCopy == null)
                return;

            _oneCopyHeight = firstCopy.ActualHeight;

            if (_oneCopyHeight <= 0)
                return;
            LaneScrollAnim.From = -_oneCopyHeight;
            LaneScrollAnim.To = 0;
            LaneScrollAnim.Duration = new Duration(TimeSpan.FromSeconds(1.0));
        }


        private void StartLaneScrollIfNeeded()
        {
            if (_laneScrollStarted) return;
            if (_oneCopyHeight <= 0) return;

            // Begin controllably ONCE, then Pause/Resume + SpeedRatio changes won’t restart it
            LaneScrollStoryboard.Begin();
            _laneScrollStarted = true;
        }

        private void UpdateRoadScrollSpeed()
        {
            if (LaneScrollStoryboard == null) return;

            // ensure started once (in case speed arrives after load)
            StartLaneScrollIfNeeded();

            if (!_laneScrollStarted) return;

            if (SpeedValue < 1.0)
            {
                LaneScrollStoryboard.Pause();
                return;
            }

            var ratio = SpeedValue / 60.0;
            ratio = Math.Clamp(ratio, 0.15, 8.0);

            LaneScrollStoryboard.SpeedRatio = ratio;
            LaneScrollStoryboard.Resume();
        }

        // =========================
        // RPM DIAL helpers
        // (expects these XAML names exist: RpmTrackPath, RpmRedlinePath, RpmValuePath, RpmNeedleRotate)
        // =========================
        private void UpdateRpmGauge(double rpmValue)
        {
            if (RpmTrackPath is null || RpmRedlinePath is null || RpmNeedleRotate is null)
                return;

            // Build the fixed dial geometry ONCE (so the coloured arcs never change)
            if (!_rpmDialBuilt)
            {
                var center = new Point(95, 78);
                double radius = 70;

                var start = StartAngleDeg;
                var endFull = StartAngleDeg + SweepDeg;

                RpmTrackPath.Data = CreateArcPolylineGeometry(center, radius, start, endFull);

                var redStart = StartAngleDeg + SweepDeg * RedlineStartRatio;
                RpmRedlinePath.Data = CreateArcPolylineGeometry(center, radius, redStart, endFull);

                _rpmDialBuilt = true;
            }

            var rpm = Math.Clamp(rpmValue, 0.0, MaxRpm);
            var ratio = rpm / MaxRpm;

            // FULL 270° sweep, no negative angles, no wrap.
            RpmNeedleRotate.Angle = 180.0 * ratio;
        }



        private static Geometry CreateArcPolylineGeometry(Point center, double radius, double startAngleDeg, double endAngleDeg)
        {
            // More points = smoother arc
            const int segments = 120;

            var startRad = DegToRad(startAngleDeg);
            var endRad = DegToRad(endAngleDeg);

            var total = endRad - startRad;
            if (total < 0)
            {
                total = 0;
            }

            var p0 = new Point(
            center.X + radius * Math.Cos(startRad),
            center.Y + radius * Math.Sin(startRad));

            var figure = new PathFigure
            {
                StartPoint = p0,
                IsClosed = false,
                IsFilled = false
            };

            var poly = new PolyLineSegment();

            for (var i = 1; i <= segments; i++)
            {
                var t = (double)i / segments;
                var a = startRad + total * t;

                poly.Points.Add(new Point(
                    center.X + radius * Math.Cos(a),
                    center.Y + radius * Math.Sin(a)));
            }

            figure.Segments.Add(poly);

            var geo = new PathGeometry();
            geo.Figures.Add(figure);
            return geo;
        }

        private static double DegToRad(double deg) => (Math.PI / 180.0) * deg;

    }
}