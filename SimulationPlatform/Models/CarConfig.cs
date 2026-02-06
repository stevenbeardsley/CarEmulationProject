using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI;

namespace SimulationPlatform.Models
{
    public class CarConfig : INotifyPropertyChanged
    {
        private Brush _colour = new SolidColorBrush(Colors.Blue);

        public event PropertyChangedEventHandler? PropertyChanged;

        public Brush Colour
        {
            get => _colour;
            set
            {
                if (_colour != value)
                {
                    _colour = value;
                    OnPropertyChanged();
                }
            }
        }

        protected void OnPropertyChanged([CallerMemberName] string? name = null)
            => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }
}
