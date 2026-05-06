using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.Foundation;
using Windows.UI;

namespace SimulationPlatform.Controls
{
    public sealed partial class TelemetryChartControl : UserControl
    {
        private const int MaxPoints = 450;
        
        private const double PadTop = 10;
        private const double PadBot = 10;
        private const int GridRows = 4;   // number of horizontal grid divisions

        private readonly Polyline _line = new();
        private readonly Polygon _fill = new();

        private readonly Queue<double> _data = new();
        private string _unit = string.Empty;
        
        private bool _fixedScale = false;
        private double _fixedMin = 0;
        private double _fixedMax = 100;

        private readonly Color _gridRuleColour = Color.FromArgb(40, 0, 0, 0);
        private readonly Color _gridLabelColour = Color.FromArgb(100, 0, 0, 0);

        public TelemetryChartControl()
        {
            InitializeComponent();

            _fill.StrokeThickness = 0;
            ChartCanvas.Children.Add(_fill);

            _line.StrokeThickness = 2.0;
            _line.StrokeLineJoin = PenLineJoin.Round;
            _line.StrokeStartLineCap = PenLineCap.Round;
            _line.StrokeEndLineCap = PenLineCap.Round;
            ChartCanvas.Children.Add(_line);
        }

        /// <summary>
        /// Call once after construction to configure appearance and Y-axis range.
        /// Pass fixedMin/fixedMax to lock the Y axis; omit them for auto-scaling.
        /// </summary>
        public void Configure(
            string title,
            Color color,
            string unit = "",
            double fixedMin = 0,
            double fixedMax = 0)
        {
            _unit = unit;

            TitleText.Text = title;          
            UnitText.Text = unit;           

            var brush = new SolidColorBrush(color);
            ValueText.Foreground = brush;
            _line.Stroke = brush;

            _fill.Fill = new SolidColorBrush(Color.FromArgb(30, color.R, color.G, color.B));

            if (fixedMax > fixedMin)
            {
                _fixedScale = true;
                _fixedMin = fixedMin;
                _fixedMax = fixedMax;

                // Write static axis labels immediately (visible before first packet)
                MinText.Text = $"{_fixedMin:0.#} {_unit}";
                MaxText.Text = $"{_fixedMax:0.#} {_unit}";
            }
        }

        /// <summary>
        /// Push a new telemetry value. Must be called on the UI thread
        /// (TelemetryPage marshals via DispatcherQueue).
        /// </summary>
        public void AddDataPoint(double value)
        {
            _data.Enqueue(value);
            if (_data.Count > MaxPoints)
                _data.Dequeue();

            ValueText.Text = $"{value:0.#}";
            Redraw();
        }


        private void ChartCanvas_SizeChanged(object sender, SizeChangedEventArgs e) => Redraw();

        private void Redraw()
        {
            var w = ChartCanvas.ActualWidth;
            var h = ChartCanvas.ActualHeight;

            if (w < 10 || h < 10) return;

            double minVal, maxVal;

            if (_fixedScale)
            {
                minVal = _fixedMin;
                maxVal = _fixedMax;
            }
            else
            {
                if (_data.Count < 2) return;
                var pts = _data.ToArray();
                minVal = pts.Min();
                maxVal = pts.Max();
                if (maxVal - minVal < 1.0) { minVal -= 5; maxVal += 5; }

                MinText.Text = $"{minVal:0.#} {_unit}";
                MaxText.Text = $"{maxVal:0.#} {_unit}";
            }

            var range = maxVal - minVal;
            if (range <= 0) return;

            var dynamic = ChartCanvas.Children
                .Where(c => c != _fill && c != _line)
                .ToList();
            foreach (var item in dynamic)
                ChartCanvas.Children.Remove(item);

            var drawH = h - PadTop - PadBot;
            for (int i = 1; i < GridRows; i++)
            {
                var ratio = i / (double)GridRows;
                var y = PadTop + drawH * ratio;
                var gridValue = maxVal - ratio * range;

                var rule = new Line
                {
                    X1 = 0,
                    Y1 = y,
                    X2 = w,
                    Y2 = y,
                    Stroke = new SolidColorBrush(_gridRuleColour),
                    StrokeThickness = 1,
                    StrokeDashArray = new DoubleCollection { 4, 4 }  // dashed suits light bg
                };

                var label = new TextBlock
                {
                    Text = $"{gridValue:0.#}",
                    Foreground = new SolidColorBrush(_gridLabelColour),
                    FontSize = 9
                };
                Canvas.SetLeft(label, 4);
                Canvas.SetTop(label, y - 10);

                ChartCanvas.Children.Insert(0, label);
                ChartCanvas.Children.Insert(0, rule);
            }

            if (_data.Count < 2) return;

            var dataArr = _data.ToArray();

            // stepX is based on MaxPoints so the graph scrolls at a consistent
            // speed regardless of how many samples are currently in the buffer.
            var stepX = w / (MaxPoints - 1.0);

            _line.Points.Clear();
            _fill.Points.Clear();

            double firstX = 0, lastX = 0;

            for (int i = 0; i < dataArr.Length; i++)
            {
                var clamped = Math.Clamp(dataArr[i], minVal, maxVal);

                // Newest point always at the right edge; older points to the left.
                var x = w - (dataArr.Length - 1 - i) * stepX;
                var y = PadTop + drawH * (1.0 - (clamped - minVal) / range);

                _line.Points.Add(new Point(x, y));
                _fill.Points.Add(new Point(x, y));

                if (i == 0) firstX = x;
                if (i == dataArr.Length - 1) lastX = x;
            }

            // Close the fill polygon along the bottom of the canvas.
            _fill.Points.Add(new Point(lastX, h - PadBot));
            _fill.Points.Add(new Point(firstX, h - PadBot));
        }
    }
}
