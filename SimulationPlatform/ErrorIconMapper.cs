using Microsoft.UI.Xaml.Controls;
using System.Collections.Generic;

namespace SimulationPlatform
{
    public static class ErrorIconMapper
    {
        // Dictionary mapping Error Codes (int) to WinUI Symbols
        private static readonly Dictionary<int, Symbol> _iconMap = new()
        {
            { 1, Symbol.Important },      // Engine No Fuel
            { 2, Symbol.Cancel },         // Engine overheating 
            { 3, Symbol.Target },         // Overheating
            { 4, Symbol.Setting },        // ECU Malfunction
            { 5, Symbol.World }           // Network/NEON Error
        };

        public static Symbol GetIcon(int code)
        {
            // Default to a generic Warning icon if code isn't found
            return _iconMap.TryGetValue(code, out var symbol) ? symbol : Symbol.Undo;
        }
    }
}