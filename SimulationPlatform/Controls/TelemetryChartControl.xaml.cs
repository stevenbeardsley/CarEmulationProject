using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;

namespace SimulationPlatform.Controls
{
    public sealed partial class TelemetryChartControl : UserControl
    {
        // ── Configuration ────────────────────────────────────────────────────
        private const int MaxPoints = 120;
        private const double PadTop = 10;
        private const double PadBot = 10;
        private const int GridRows = 4;

        // ── Persistent canvas children (never removed during redraw) ─────────
        private readonly Polyline _line = new();
        private readonly Polygon _fill = new();

        // ── Data ─────────────────────────────────────────────────────────────
        private readonly Queue<double> _data = new();
        private string _unit = string.Empty;

        public TelemetryChartControl()
        {
            InitializeComponent();

            _fill.StrokeThickness = 0;
            ChartCanvas.Children.Add(_fill);

            _line.StrokeThickness = 1.8;
            _line.StrokeLineJoin = PenLineJoin.Round;
            _line.StrokeStartLineCap = PenLineCap.Round;
            _line.StrokeEndLineCap = PenLineCap.Round;
            ChartCanvas.Children.Add(_line);
        }

        // ── Public API ───────────────────────────────────────────────────────

        public void Configure(string title, Color color, string unit = "")
        {
            _unit = unit;

            TitleText.Text = title.ToUpperInvariant();
            UnitText.Text = unit;

            var brush = new SolidColorBrush(color);
            ValueText.Foreground = brush;
            _line.Stroke = brush;
            _fill.Fill = new SolidColorBrush(Color.FromArgb(38, color.R, color.G, color.B));
        }

        /// <summary>
        /// Push a new value. Must be called on the UI thread
        /// (TelemetryWindow marshals via DispatcherQueue).
        /// </summary>
        public void AddDataPoint(double value)
        {
            _data.Enqueue(value);
            if (_data.Count > MaxPoints)
                _data.Dequeue();

            ValueText.Text = $"{value:0.#}";
            Redraw();
        }

        // ── Drawing ──────────────────────────────────────────────────────────

        private void ChartCanvas_SizeChanged(object sender, SizeChangedEventArgs e) => Redraw();

        private void Redraw()
        {
            var w = ChartCanvas.ActualWidth;
            var h = ChartCanvas.ActualHeight;

            if (w < 10 || h < 10 || _data.Count < 2) return;

            var pts = _data.ToArray();
            var minVal = pts.Min();
            var maxVal = pts.Max();
            var range = maxVal - minVal;

            if (range < 1.0) { minVal -= 5; maxVal += 5; range = maxVal - minVal; }

            MinText.Text = $"{minVal:0.#} {_unit}";
            MaxText.Text = $"{maxVal:0.#} {_unit}";

            // ── Remove every dynamic child, keep only _fill and _line ────────
            // Collect items to remove first to avoid modifying the collection
            // while iterating.
            var dynamic = ChartCanvas.Children
                .Where(c => c != _fill && c != _line)
                .ToList();
            foreach (var item in dynamic)
                ChartCanvas.Children.Remove(item);

            // ── Grid rules ───────────────────────────────────────────────────
            var drawH = h - PadTop - PadBot;
            for (int i = 1; i < GridRows; i++)
            {
                var ratio = i / (double)GridRows;
                var y = PadTop + drawH * ratio;

                var rule = new Line
                {
                    X1 = 0,
                    Y1 = y,
                    X2 = w,
                    Y2 = y,
                    Stroke = new SolidColorBrush(Color.FromArgb(30, 255, 255, 255)),
                    StrokeThickness = 1
                };

                var label = new TextBlock
                {
                    Text = $"{maxVal - ratio * range:0.#}",
                    Foreground = new SolidColorBrush(Color.FromArgb(60, 255, 255, 255)),
                    FontSize = 8
                };
                Canvas.SetLeft(label, 4);
                Canvas.SetTop(label, y - 9);

                // Insert behind _fill and _line (index 0)
                ChartCanvas.Children.Insert(0, label);
                ChartCanvas.Children.Insert(0, rule);
            }

            // ── Polyline + fill ───────────────────────────────────────────────
            var stepX = w / (MaxPoints - 1.0);

            _line.Points.Clear();
            _fill.Points.Clear();

            double firstX = 0, lastX = 0;

            for (int i = 0; i < pts.Length; i++)
            {
                var x = w - (pts.Length - 1 - i) * stepX;
                var y = PadTop + drawH * (1.0 - (pts[i] - minVal) / range);

                _line.Points.Add(new Point(x, y));
                _fill.Points.Add(new Point(x, y));

                if (i == 0) firstX = x;
                if (i == pts.Length - 1) lastX = x;
            }

            _fill.Points.Add(new Point((float)lastX, h - PadBot));
            _fill.Points.Add(new Point((float)firstX, h - PadBot));
        }
    }
}
