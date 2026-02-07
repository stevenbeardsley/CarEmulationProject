using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI;

namespace SimulationPlatform.Models
{
    public class CarConfig : INotifyPropertyChanged
    {
        private Brush m_colour = new SolidColorBrush(Colors.Blue);
        private int m_gearsCount = 0;

        public event PropertyChangedEventHandler? PropertyChanged;

        public Brush Colour
        {
            get => m_colour;
            set
            {
                if (m_colour != value)
                {
                    m_colour = value;
                    OnPropertyChanged();
                }
            }
        }
        
        public int GearsCount
        {
            get => m_gearsCount;
            set
            {
                if (m_gearsCount != value)
                {
                    m_gearsCount = value;
                    OnPropertyChanged();
                }

            }
        }

        protected void OnPropertyChanged([CallerMemberName] string? name = null)
            => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }
}
